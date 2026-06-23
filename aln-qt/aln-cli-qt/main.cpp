#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QTimer>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

#include <aln/router.h>
#include <aln/packet.h>
#include <aln/tcpchannel.h>

#include <csignal>

static QCoreApplication* appInstance = nullptr;

static void signalHandler(int)
{
    if (appInstance)
        QMetaObject::invokeMethod(appInstance, "quit", Qt::QueuedConnection);
}

// --- advertise command ---

static int cmdAdvertise(QCoreApplication &app, const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Broadcast a UDP advertisement periodically");
    parser.addHelpOption();
    parser.addOption({{"a", "address"}, "Broadcast address (e.g. 192.168.1.255)", "address"});
    parser.addOption({{"p", "port"}, "UDP port (default: 8082)", "port", "8082"});
    parser.addOption({"period", "Broadcast period in seconds (default: 5)", "seconds", "5"});
    parser.addOption({{"m", "message"}, "Message to advertise (e.g. tcp+aln://192.168.1.5:8081)", "text"});
    parser.process(args);

    if (!parser.isSet("address") || !parser.isSet("message")) {
        QTextStream err(stderr);
        err << "Error: --address and --message are required\n\n";
        parser.showHelp(1);
    }

    QString address = parser.value("address");
    int port = parser.value("port").toInt();
    int periodMs = parser.value("period").toInt() * 1000;
    QString message = parser.value("message");
    QByteArray dgram = message.toUtf8();

    QTextStream out(stdout);
    out << "Advertising '" << message << "' to " << address << ":" << port
        << " every " << (periodMs / 1000) << "s\n";
    out.flush();

    QUdpSocket udp;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        udp.writeDatagram(dgram, QHostAddress(address), port);
    });
    // Send immediately, then on timer
    udp.writeDatagram(dgram, QHostAddress(address), port);
    timer.start(periodMs);

    return app.exec();
}

// --- listen command ---

static int cmdListen(QCoreApplication &app, const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Listen for UDP broadcast advertisements");
    parser.addHelpOption();
    parser.addOption({{"a", "address"}, "Broadcast address to listen on", "address"});
    parser.addOption({{"p", "port"}, "UDP port (default: 8082)", "port", "8082"});
    parser.process(args);

    if (!parser.isSet("address")) {
        QTextStream err(stderr);
        err << "Error: --address is required\n\n";
        parser.showHelp(1);
    }

    QString address = parser.value("address");
    int port = parser.value("port").toInt();

    QTextStream out(stdout);
    out << "Listening for broadcasts on port " << port << "\n";
    out.flush();

    QUdpSocket udp;
    if (!udp.bind(port, QUdpSocket::ShareAddress)) {
        QTextStream err(stderr);
        err << "Error: failed to bind to port " << port << "\n";
        return 1;
    }

    QObject::connect(&udp, &QUdpSocket::readyRead, [&]() {
        while (udp.hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(int(udp.pendingDatagramSize()));
            QHostAddress sender;
            quint16 senderPort;
            udp.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            QTextStream out(stdout);
            out << sender.toString() << ":" << senderPort << " -> " << QString::fromUtf8(datagram) << "\n";
            out.flush();
        }
    });

    return app.exec();
}

// --- send command ---

static int cmdSend(QCoreApplication &app, const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Send a packet to a destination");
    parser.addHelpOption();
    parser.addOption({{"c", "connect"}, "Connection URL (e.g. tcp+aln://192.168.1.5:8081)", "url"});
    parser.addOption({{"d", "dest"}, "Destination node address", "address"});
    parser.addOption({{"s", "service"}, "Service name", "name"});
    parser.addOption({"data", "Payload data", "payload"});
    parser.addOption({{"w", "wait"}, "Wait for a reply via context ID, then print and exit"});
    parser.process(args);

    if (!parser.isSet("connect") || !parser.isSet("service")) {
        QTextStream err(stderr);
        err << "Error: --connect and --service are required\n\n";
        parser.showHelp(1);
    }

    QString urlString = parser.value("connect");
    QString dest = parser.value("dest");
    QString service = parser.value("service");
    QByteArray data = parser.value("data").toUtf8();
    bool waitForReply = parser.isSet("wait");

    QUrl url(urlString);
    if (!url.isValid() || url.host().isEmpty()) {
        QTextStream err(stderr);
        err << "Error: invalid connection URL: " << urlString << "\n";
        return 1;
    }

    Router *router = new Router();
    QTcpSocket *socket = new QTcpSocket();
    TcpChannel *channel = new TcpChannel(socket);

    QTextStream out(stdout);
    out << "Connecting to " << urlString << "...\n";
    out.flush();

    QObject::connect(socket, &QTcpSocket::connected, [&]() {
        QTextStream out(stdout);
        out << "Connected. Sending packet to service '" << service << "'";
        if (!dest.isEmpty())
            out << " at " << dest;
        out << "\n";
        out.flush();

        router->addChannel(channel);

        // Wait briefly for route advertisements if destination specified
        int delay = dest.isEmpty() ? 0 : 200;
        QTimer::singleShot(delay, [&]() {
            Packet *packet;
            if (dest.isEmpty()) {
                packet = new Packet("", service, data);
            } else {
                packet = new Packet(dest, service, data);
            }
            packet->srcAddress = router->address();

            if (waitForReply) {
                // Lambda-based context handler using PacketHandler subclass
                class ReplyHandler : public PacketHandler {
                public:
                    QCoreApplication &app;
                    ReplyHandler(QCoreApplication &a) : app(a) {}
                    void onPacket(Packet *p) override {
                        QTextStream out(stdout);
                        out << "Reply from " << p->srcAddress << ": " << QString::fromUtf8(p->data) << "\n";
                        out.flush();
                        QTimer::singleShot(0, &app, &QCoreApplication::quit);
                    }
                };
                auto *handler = new ReplyHandler(app);
                uint16_t ctx = router->registerContextHandler(handler);
                packet->ctx = ctx;
            }

            router->send(packet);

            if (!waitForReply) {
                // Give a moment for the packet to be sent, then exit
                QTimer::singleShot(500, &app, &QCoreApplication::quit);
            }
        });
    });

    QObject::connect(socket, &QTcpSocket::errorOccurred, [&](QAbstractSocket::SocketError) {
        QTextStream err(stderr);
        err << "Connection error: " << socket->errorString() << "\n";
        err.flush();
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    });

    socket->connectToHost(url.host(), url.port(8081));

    return app.exec();
}

// --- serve command ---

static int cmdServe(QCoreApplication &app, const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Open a TCP listening port and run the router with echo/log services");
    parser.addHelpOption();
    parser.addOption({{"a", "address"}, "Bind address (default: 0.0.0.0)", "address", "0.0.0.0"});
    parser.addOption({{"p", "port"}, "Listen port (default: 8081)", "port", "8081"});
    parser.process(args);

    QString address = parser.value("address");
    int port = parser.value("port").toInt();

    Router *router = new Router();

    QTextStream out(stdout);
    out << "Node address: " << router->address() << "\n";
    out.flush();

    // Register log service - prints received packets to stdout
    class LogHandler : public PacketHandler {
    public:
        void onPacket(Packet *p) override {
            QTextStream out(stdout);
            out << "[log] from=" << p->srcAddress << " data=" << QString::fromUtf8(p->data) << "\n";
            out.flush();
        }
    };
    router->registerService("log", new LogHandler());

    // Register echo service - echoes packets back to sender
    class EchoHandler : public PacketHandler {
        Router *mRouter;
    public:
        EchoHandler(Router *r) : mRouter(r) {}
        void onPacket(Packet *p) override {
            if (p->srcAddress.isEmpty() || p->ctx == 0)
                return;
            QTextStream out(stdout);
            out << "[echo] from=" << p->srcAddress << " data=" << QString::fromUtf8(p->data) << "\n";
            out.flush();
            mRouter->send(new Packet(p->srcAddress, p->ctx, p->data));
        }
    };
    router->registerService("echo", new EchoHandler(router));

    // Open TCP server
    QTcpServer *server = new QTcpServer();
    QObject::connect(server, &QTcpServer::newConnection, [server, router]() {
        while (server->hasPendingConnections()) {
            QTcpSocket *sock = server->nextPendingConnection();
            TcpChannel *ch = new TcpChannel(sock);
            router->addChannel(ch);
            QTextStream out(stdout);
            out << "Accepted connection from " << sock->peerAddress().toString()
                << ":" << sock->peerPort() << "\n";
            out.flush();
        }
    });

    if (!server->listen(QHostAddress(address), port)) {
        QTextStream err(stderr);
        err << "Error: failed to listen on " << address << ":" << port
            << " - " << server->errorString() << "\n";
        return 1;
    }

    out << "Listening on " << address << ":" << port << "\n";
    out.flush();

    // Print route changes
    QObject::connect(router, &Router::netStateChanged, [router]() {
        QTextStream out(stdout);
        QMap<QString, QStringList> nodes = router->nodeServices();
        out << "--- Network state ---\n";
        for (auto it = nodes.constBegin(); it != nodes.constEnd(); ++it) {
            out << "  " << it.key() << " services=[" << it.value().join(", ") << "]\n";
        }
        out << "---\n";
        out.flush();
    });

    return app.exec();
}

// --- main ---

static void printUsage()
{
    QTextStream out(stdout);
    out << "aln-wrench-cli - ALN developer utility (headless)\n\n"
        << "Usage: aln-wrench-cli <command> [options]\n\n"
        << "Commands:\n"
        << "  advertise   Broadcast a UDP advertisement periodically\n"
        << "  listen      Listen for UDP broadcast advertisements\n"
        << "  send        Send a packet to a destination\n"
        << "  serve       Open a TCP listening port and run the router\n"
        << "\nRun 'aln-wrench-cli <command> --help' for command-specific options.\n";
    out.flush();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("aln-wrench-cli");
    app.setApplicationVersion("1.0");
    appInstance = &app;

    // Install signal handlers for clean shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    QStringList args = app.arguments();

    if (args.size() < 2) {
        printUsage();
        return 1;
    }

    QString command = args.at(1);

    // Build a sub-argument list: program name + everything after the command
    QStringList subArgs;
    subArgs << args.at(0);
    for (int i = 2; i < args.size(); ++i)
        subArgs << args.at(i);

    if (command == "advertise")
        return cmdAdvertise(app, subArgs);
    else if (command == "listen")
        return cmdListen(app, subArgs);
    else if (command == "send")
        return cmdSend(app, subArgs);
    else if (command == "serve")
        return cmdServe(app, subArgs);
    else if (command == "--help" || command == "-h") {
        printUsage();
        return 0;
    } else {
        QTextStream err(stderr);
        err << "Unknown command: " << command << "\n\n";
        printUsage();
        return 1;
    }
}
