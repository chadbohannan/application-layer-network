#include "addchanneldialog.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aln/packet.h"
#include "aln/parser.h"
#include "aln/ax25frame.h"

#include <QNetworkInterface>
#include <QTcpSocket>
#include <QUuid>

#include <aln/localchannel.h>

class TestPacketHandler : public PacketHandler {
    bool* testFlag = 0;
public:
    TestPacketHandler(bool* tf) {
        testFlag = tf;
    }
    void onPacket(Packet *) {
        *testFlag = true;
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->addChannelButton, SIGNAL(clicked()), this, SLOT(onAddChannelButtonClicked()));

    // smoke test of the ALN implementation
    Packet p1("target", 42, "test");
    QByteArray p1bytes = p1.toByteArray();
    Packet p2(p1bytes);
    assert(p2.toByteArray() == p1bytes);

    testFlag = false;
    Parser parser = Parser();
    connect(&parser, &Parser::onPacket,
            this, &MainWindow::onPacket);
    parser.read(toAx25Buffer(p1bytes));
    assert(testFlag);

    alnRouter = new Router();
    Router* testRouter1 = new Router("test-router-1");
    LocalChannel* lc1 = new LocalChannel();
    alnRouter->addChannel(lc1);
    testRouter1->addChannel(lc1->buddy());

    Router* testRouter2 = new Router("test-router-2");
    LocalChannel* lc2 = new LocalChannel();
    testRouter1->addChannel(lc2);
    testRouter2->addChannel(lc2->buddy());

    testRouter2->registerService("test", new TestPacketHandler(&testFlag));
    testFlag = false;
    qDebug() << "onSend:" << alnRouter->send(new Packet("", "test", QByteArray()));

    this->statusBar()->showMessage("Application Layer Network Node Address: " + alnRouter->address());

    // TODO compose list model for local interfaces widget
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface iFace, allInterfaces) {
        QNetworkAddressEntry entry = iFace.addressEntries().first();
        QHostAddress address = entry.ip();
        switch(iFace.type()){
        case QNetworkInterface::InterfaceType::Ethernet:
            qDebug() << QString("%1 - Ethernet").arg(address.toString());
            break;
        case QNetworkInterface::InterfaceType::Wifi:
            qDebug() << QString("%1 - Wifi").arg(address.toString());
            break;
        default:
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onAddChannelButtonClicked() {
    AddChannelDialog* dialog = new AddChannelDialog(this);
    connect(dialog, SIGNAL(connectRequest(QString)), this, SLOT(onConnectRequest(QString)));
    dialog->show();
}

void MainWindow::onConnectRequest(QString urlString) {
    QUrl url = QUrl::fromEncoded(urlString.toUtf8());
    if (url.scheme() == "tcp+maln") {
        QTcpSocket* socket = new QTcpSocket(this);
        socket->connectToHost(url.host(), url.port());
        TcpChannel* channel = new TcpChannel(socket);
        QString malnAddr = url.scheme().remove("/");
        channel->send(new Packet(malnAddr, QByteArray()));
        channels.append(channel);

    } else if (url.scheme() == "tls+maln") {
        // TODO QSslSocket
    } else if (url.scheme() == "tcp+aln") {
        QTcpSocket* socket = new QTcpSocket(this);
        socket->connectToHost(url.host(), url.port());
        channels.append(new TcpChannel(socket));
    } else if (url.scheme() == "tls+aln") {
        // TODO QSslSocket
    }
}

void MainWindow::onPacket(Packet* p) {
    testFlag = true;
}
