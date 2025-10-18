#include "addchanneldialog.h"
#include "mainwindow.h"
#include "packetsenddialog.h"
#include "ui_mainwindow.h"

#include "aln/packet.h"
#include <aln/localchannel.h>

#include <QDateTime>
#include <QNetworkInterface>
#include <QSslSocket>
#include <QStringListModel>
#include <QUuid>


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

class PacketSignaler: public PacketHandler {
public:
    PacketSignaler() { }
    void onPacket(Packet *packet) {
        emit packetReceived(packet);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this, SIGNAL(logLineReady(QString)), SLOT(addLogLine(QString)), Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    // Clean up advertisements
    foreach(AdvertisementItem* ad, advertisements) {
        delete ad;
    }
    // Clean up discovery listeners
    foreach(DiscoveryListenerItem* listener, discoveryListeners) {
        delete listener;
    }
    delete ui;
}

void MainWindow::init() {
    connect(ui->openPortButton, SIGNAL(clicked()),
            this, SLOT(onOpenPortButtonClicked()));
    connect(ui->closePortButton, SIGNAL(clicked()),
            this, SLOT(onClosePortButtonClicked()));

    alnRouter = new Router();
    PacketSignaler* logSignaler = new PacketSignaler();
    connect(logSignaler, SIGNAL(packetReceived(Packet*)), this, SLOT(logServicePacketHandler(Packet*)));
    alnRouter->registerService("log", logSignaler);
    connect(ui->clearLogButton, SIGNAL(clicked()), this, SLOT(clearLog()));

    PacketSignaler* echoSignaler = new PacketSignaler();
    connect(echoSignaler, SIGNAL(packetReceived(Packet*)), this, SLOT(echoServicePacketHandler(Packet*)));
    alnRouter->registerService("echo", echoSignaler );

    connect(alnRouter, SIGNAL(netStateChanged()),
            this, SLOT(onNetStateChanged()));
    selfTest();

    populateNetworkInterfaces();

    // Initialize known connections table
    ui->knownConnectionsTableView->setModel(new KnownConnectionItemModel(knownConnections, this));
    ui->knownConnectionsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->knownConnectionsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->knownConnectionsTableView->horizontalHeader()->setSectionResizeMode(KnownConnectionColumn::ConnectionUrl, QHeaderView::Stretch);
    ui->knownConnectionsTableView->horizontalHeader()->setSectionResizeMode(KnownConnectionColumn::ConnectionStatusColumn, QHeaderView::ResizeToContents);

    connect(ui->addConnectionButton, SIGNAL(clicked()),
            this, SLOT(onAddConnectionButtonClicked()));
    connect(ui->connectButton, SIGNAL(clicked()),
            this, SLOT(onConnectButtonClicked()));
    connect(ui->disconnectButton, SIGNAL(clicked()),
            this, SLOT(onDisconnectButtonClicked()));
    connect(ui->knownConnectionsTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onKnownConnectionSelectionChanged()));

    connect(&serviceButtonGroup, SIGNAL(idClicked(int)),
            this, SLOT(onServiceButtonClicked(int)));

    // Initialize advertisement table
    ui->advertisementsTableView->setModel(new AdvertisementItemModel(advertisements, this));
    ui->advertisementsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->advertisementsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdMessage, QHeaderView::Stretch);
    ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdPeriod, QHeaderView::ResizeToContents);
    ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdStarted, QHeaderView::ResizeToContents);

    connect(ui->startAdvertisementButton, SIGNAL(clicked()),
            this, SLOT(onStartAdvertisementButtonClicked()));
    connect(ui->stopAdvertisementButton, SIGNAL(clicked()),
            this, SLOT(onStopAdvertisementButtonClicked()));
    connect(ui->advertisementsTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onAdvertisementSelectionChanged()));

    // Initialize discovery listeners table
    ui->discoveryListenersTableView->setModel(new DiscoveryListenerItemModel(discoveryListeners, this));
    ui->discoveryListenersTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->discoveryListenersTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->discoveryListenersTableView->horizontalHeader()->setSectionResizeMode(DiscoveryListenerColumn::ListenerAddress, QHeaderView::Stretch);
    ui->discoveryListenersTableView->horizontalHeader()->setSectionResizeMode(DiscoveryListenerColumn::ListenerPort, QHeaderView::ResizeToContents);

    connect(ui->discoveryListenersTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onDiscoveryListenerSelectionChanged()));

    // Network broadcast discovery buttons
    connect(ui->startListenButton, SIGNAL(clicked()),
            this, SLOT(onStartListenButtonClicked()));
    connect(ui->stopListenButton, SIGNAL(clicked()),
            this, SLOT(onStopListenButtonClicked()));

    // bluetooth discovery
    btDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(btDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    connect(ui->btDiscoveryGroupBox, SIGNAL(toggled(bool)),
            this, SLOT(btDiscoveryCheckboxChanged(bool)));
    connect(&btConnectButtonGroup, SIGNAL(idClicked(int)),
            this, SLOT(onBtConnectButtonClicked(int)));

    ui->btDiscoveryGroupBox->setVisible(false);

    this->statusBar()->showMessage("This node's Application Layer Network address: " + alnRouter->address());

    serviceUuidToNameMap.insert("94f39d29-7d6d-437d-973b-fba39e49d4ee", "rfcomm-client");
    serviceUuidToNameMap.insert("00001101-0000-1000-8000-10ca10ddba11", "blinky-bt");
    serviceUuidToNameMap.insert("00001101-0000-1000-8000-00805F9B34FB", "serial adapter");
    serviceUuidToNameMap.insert("00001101-0000-1000-8000-00805f9b34fb", "serial adapter");
}

void MainWindow::logServicePacketHandler(Packet* packet) {
    while (logServiceBufferList.size() > 20)
        logServiceBufferList.removeFirst();
    logServiceBufferList.append(QString("%0 - %1").arg(packet->srcAddress).arg(packet->data));
    ui->logServiceListView->setModel(new QStringListModel(logServiceBufferList));
}

void MainWindow::echoServicePacketHandler(Packet* packet) {
    if (packet->srcAddress.length() == 0) {
        qWarning() << "echo service cannot respond to an empty address";
        return;
    }
    if (packet->ctx == 0) {
        qWarning() << "echo service cannot respond to null context (context id is zero)";
        return;
    }
    qDebug() << "echo service returning " << packet->data << " to " << packet->srcAddress << ":" << packet->ctx;
    alnRouter->send(new Packet(packet->srcAddress, packet->ctx, packet->data));
}

void MainWindow::addLogLine(QString msg) {
    while (debugBufferList.size() > 20)
        debugBufferList.removeFirst();
    debugBufferList.append(msg);
    ui->statusLogListView->setModel(new QStringListModel(debugBufferList));
}

void MainWindow::clearLog() {
    debugBufferList.clear();
    ui->statusLogListView->setModel(new QStringListModel(debugBufferList));
}

void MainWindow::onDebugMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString line;

    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        if (ui->logDebugCheckBox->isChecked())
            emit logLineReady(QString("%0 DEBUG %1").arg(QDateTime::currentDateTime().toString( Qt::ISODateWithMs)).arg(msg));
        break;
    case QtInfoMsg:
        if (ui->logInfoCheckBox->isChecked())
            emit logLineReady(QString("%0 INFO %1").arg(QDateTime::currentDateTime().toString( Qt::ISODateWithMs)).arg(msg));
        break;
    case QtWarningMsg:
        if (ui->logErrorCheckBox->isChecked())
            emit logLineReady(QString("%0 WARNING %1").arg(QDateTime::currentDateTime().toString( Qt::ISODateWithMs)).arg(msg));
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        if (ui->logErrorCheckBox->isChecked())
            emit logLineReady(QString("%0 CRITICAL %1").arg(QDateTime::currentDateTime().toString( Qt::ISODateWithMs)).arg(msg));
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}

void MainWindow::selfTest() {
    // smoke test of the ALN implementation
    Packet p1("target", 42, "test");
    QByteArray p1bytes = p1.toByteArray();
    Packet p2(p1bytes);
    assert(p2.toByteArray() == p1bytes);

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

    QThread::msleep(10);

    lc1->disconnect();
    lc2->disconnect();
    QThread::msleep(1);

    lc1->deleteLater();
    lc2->deleteLater();
    QThread::msleep(1);

    testRouter1->deleteLater();
    testRouter2->deleteLater();
}

void MainWindow::populateNetworkInterfaces() {
    // Start with empty table - ports will be added via "Open Port" button
    niim = new NetworkInterfacesItemModel(interfaces, this);
    ui->networkInterfacesTableView->setModel(niim);
    ui->networkInterfacesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->networkInterfacesTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Type, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Scheme, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Address, QHeaderView::Stretch);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::ListenPort, QHeaderView::ResizeToContents);

    // Connect selection changes to enable/disable close button
    connect(ui->networkInterfacesTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onNetworkInterfaceSelectionChanged()));
}

void MainWindow::onOpenPortButtonClicked() {
    // Discover available network interfaces
    QStringList availableInterfaces;
    QMap<QString, QString> interfaceToBroadcastMap;
    QMap<QString, QString> interfaceToTypeMap;

    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);

    foreach(QNetworkInterface iFace, allInterfaces) {
        QList<QNetworkAddressEntry> entries = iFace.addressEntries();
        foreach(QNetworkAddressEntry entry, entries) {
            QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
                QString addrStr = address.toString();
                switch(iFace.type()){
                case QNetworkInterface::InterfaceType::Ethernet:
                    availableInterfaces << addrStr;
                    interfaceToBroadcastMap.insert(addrStr, entry.broadcast().toString());
                    interfaceToTypeMap.insert(addrStr, "eth");
                    break;
                case QNetworkInterface::InterfaceType::Wifi:
                    availableInterfaces << addrStr;
                    interfaceToBroadcastMap.insert(addrStr, entry.broadcast().toString());
                    interfaceToTypeMap.insert(addrStr, "wifi");
                    break;
                default:
                    break;
                }
            }
        }
    }

    if (availableInterfaces.isEmpty()) {
        qWarning() << "No network interfaces available";
        return;
    }

    // Show dialog
    OpenPortDialog* dialog = new OpenPortDialog(availableInterfaces, this);
    if (dialog->exec() == QDialog::Accepted) {
        QString interface = dialog->selectedInterface();
        int port = dialog->selectedPort();

        // Check if this interface:port combo already exists
        bool alreadyOpen = false;
        foreach(NetworkInterfaceItem* item, interfaces) {
            if (item->listenAddress() == interface && item->listenPort() == port) {
                qWarning() << "Port" << port << "already open on" << interface;
                alreadyOpen = true;
                break;
            }
        }

        if (!alreadyOpen) {
            // Create new interface item
            QString type = interfaceToTypeMap.value(interface, "unknown");
            QString broadcastAddr = interfaceToBroadcastMap.value(interface, "");
            NetworkInterfaceItem* newItem = new NetworkInterfaceItem(type, interface, port, broadcastAddr);
            newItem->setIsListening(true);

            // Add to interfaces list
            interfaces.append(newItem);

            // Update the model
            QItemSelectionModel *oldSelectionModel = ui->networkInterfacesTableView->selectionModel();
            ui->networkInterfacesTableView->setModel(new NetworkInterfacesItemModel(interfaces, this));
            if (oldSelectionModel) delete oldSelectionModel;

            ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Type, QHeaderView::ResizeToContents);
            ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Scheme, QHeaderView::ResizeToContents);
            ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Address, QHeaderView::Stretch);
            ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::ListenPort, QHeaderView::ResizeToContents);

            // Reconnect selection handler after model change
            connect(ui->networkInterfacesTableView->selectionModel(),
                    SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    this, SLOT(onNetworkInterfaceSelectionChanged()));

            // Actually open the port
            onListenRequest("tcp+aln", interface, port, true);
        }
    }

    dialog->deleteLater();
}

void MainWindow::onNetworkInterfaceSelectionChanged() {
    // Enable Close Port button only when a row is selected
    QModelIndexList selectedRows = ui->networkInterfacesTableView->selectionModel()->selectedRows();
    ui->closePortButton->setEnabled(!selectedRows.isEmpty());
}

void MainWindow::onClosePortButtonClicked() {
    QModelIndexList selectedRows = ui->networkInterfacesTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    int row = selectedRows.first().row();
    if (row < 0 || row >= interfaces.size()) {
        qWarning() << "Invalid row selected:" << row;
        return;
    }

    NetworkInterfaceItem* item = interfaces.at(row);
    QString interface = item->listenAddress();
    int port = item->listenPort();

    // Close the listening port
    onListenRequest("tcp+aln", interface, port, false);

    // Remove from interfaces list
    interfaces.removeAt(row);
    delete item;

    // Update the model
    QItemSelectionModel *oldSelectionModel = ui->networkInterfacesTableView->selectionModel();
    ui->networkInterfacesTableView->setModel(new NetworkInterfacesItemModel(interfaces, this));
    if (oldSelectionModel) delete oldSelectionModel;

    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Type, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Scheme, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Address, QHeaderView::Stretch);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::ListenPort, QHeaderView::ResizeToContents);

    // Reconnect selection handler after model change
    connect(ui->networkInterfacesTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onNetworkInterfaceSelectionChanged()));

    // Disable the close button since selection is now cleared
    ui->closePortButton->setEnabled(false);

    qInfo() << "Closed port" << port << "on interface" << interface;
}

void MainWindow::onAddConnectionButtonClicked() {
    AddChannelDialog* dialog = new AddChannelDialog(this);
    // When user enters a URL in the dialog, add it to known connections
    connect(dialog, &AddChannelDialog::connectRequest, this, [this](QString url) {
        // Check if already in known connections
        KnownConnectionItem* existing = findKnownConnection(url);
        if (!existing) {
            // Add new connection as disconnected
            KnownConnectionItem* newItem = new KnownConnectionItem(url, Disconnected, nullptr);
            knownConnections.append(newItem);
            updateKnownConnectionsTable();
            qInfo() << "Added connection:" << url;
        }
        // Auto-connect
        onConnectButtonClicked();
    });
    dialog->show();
}

KnownConnectionItem* MainWindow::findKnownConnection(QString url) {
    foreach(KnownConnectionItem* item, knownConnections) {
        if (item->url() == url) {
            return item;
        }
    }
    return nullptr;
}

void MainWindow::updateKnownConnectionsTable() {
    // Disconnect selection handler before model update
    disconnect(ui->knownConnectionsTableView->selectionModel(),
               SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
               this, SLOT(onKnownConnectionSelectionChanged()));

    // Update the model
    QItemSelectionModel *oldSelectionModel = ui->knownConnectionsTableView->selectionModel();
    ui->knownConnectionsTableView->setModel(new KnownConnectionItemModel(knownConnections, this));
    delete oldSelectionModel;

    ui->knownConnectionsTableView->horizontalHeader()->setSectionResizeMode(KnownConnectionColumn::ConnectionUrl, QHeaderView::Stretch);
    ui->knownConnectionsTableView->horizontalHeader()->setSectionResizeMode(KnownConnectionColumn::ConnectionStatusColumn, QHeaderView::ResizeToContents);

    // Reconnect selection handler after model change
    connect(ui->knownConnectionsTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onKnownConnectionSelectionChanged()));
}

void MainWindow::onKnownConnectionSelectionChanged() {
    QModelIndexList selectedRows = ui->knownConnectionsTableView->selectionModel()->selectedRows();

    if (selectedRows.isEmpty()) {
        ui->connectButton->setEnabled(false);
        ui->disconnectButton->setEnabled(false);
        return;
    }

    int row = selectedRows.first().row();
    if (row < 0 || row >= knownConnections.size()) {
        ui->connectButton->setEnabled(false);
        ui->disconnectButton->setEnabled(false);
        return;
    }

    KnownConnectionItem* item = knownConnections.at(row);
    bool isConnected = (item->status() == Connected);

    ui->connectButton->setEnabled(!isConnected);
    ui->disconnectButton->setEnabled(isConnected);
}

void MainWindow::onConnectButtonClicked() {
    QModelIndexList selectedRows = ui->knownConnectionsTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    int row = selectedRows.first().row();
    if (row < 0 || row >= knownConnections.size()) {
        qWarning() << "Invalid row selected:" << row;
        return;
    }

    KnownConnectionItem* item = knownConnections.at(row);
    if (item->status() == Connected) {
        qInfo() << "Already connected to" << item->url();
        return;
    }

    QString urlString = item->url();
    QUrl url = QUrl::fromEncoded(urlString.toUtf8());

    TcpChannel* channel;
    if (url.scheme().startsWith("tls")) {
        QSslSocket* socket = new QSslSocket(this);
        socket->connectToHostEncrypted(url.host(), url.port());
        channel = new TcpChannel(socket);
    } else {
        QTcpSocket* socket = new QTcpSocket(this);
        socket->connectToHost(url.host(), url.port());
        channel = new TcpChannel(socket);
    }

    if (url.scheme().endsWith("maln")) {
        QString malnAddr = url.path().remove("/");
        channel->send(new Packet(malnAddr, QByteArray()));
    }

    alnRouter->addChannel(channel);
    connect(channel, SIGNAL(closing(Channel*)), this, SLOT(onChannelClosing(Channel*)), Qt::QueuedConnection);

    // Update item status
    item->setStatus(Connected);
    item->setChannel(channel);
    updateKnownConnectionsTable();

    qInfo() << "Connected to" << urlString;
}

void MainWindow::onDisconnectButtonClicked() {
    QModelIndexList selectedRows = ui->knownConnectionsTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    int row = selectedRows.first().row();
    if (row < 0 || row >= knownConnections.size()) {
        qWarning() << "Invalid row selected:" << row;
        return;
    }

    KnownConnectionItem* item = knownConnections.at(row);
    if (item->status() == Disconnected) {
        qInfo() << "Already disconnected from" << item->url();
        return;
    }

    Channel* channel = item->channel();
    if (channel) {
        channel->disconnect();
    }

    // Update item status
    item->setStatus(Disconnected);
    item->setChannel(nullptr);
    updateKnownConnectionsTable();

    qInfo() << "Disconnected from" << item->url();
}

void MainWindow::onChannelClosing(Channel* ch)
{
    // Find the connection and update its status to Disconnected
    foreach(KnownConnectionItem* item, knownConnections) {
        if (item->channel() == ch) {
            item->setStatus(Disconnected);
            item->setChannel(nullptr);
            updateKnownConnectionsTable();
            qInfo() << "Connection closed:" << item->url();
            break;
        }
    }
}


void MainWindow::onListenRequest(QString scheme, QString interface, short port, bool enable) {
    QTcpServer* srvr;
    QString key = QString("%1:%2").arg(interface).arg(port);
    if (tcpServers.contains(key)) {
        srvr = tcpServers.value(key);
    } else {
        qInfo() << "creating server for "<< key;
        // TODO handle various scheme
        srvr = new QTcpServer(this);
        connect(srvr, SIGNAL(newConnection()), this,SLOT(onTcpListenPending()));
        tcpServers.insert(key, srvr);
    }
    if (enable && !srvr->isListening()) {
        qInfo() << "starting server for " << key;
        srvr->listen(QHostAddress(interface), port);
    } else if (!enable && srvr->isListening()) {
        qInfo() << "stopping server for " << key;
        srvr->close();
    }
}

void MainWindow::onTcpListenPending() {
    foreach (QTcpServer* srvr, tcpServers) {
        while (srvr->hasPendingConnections()) {
            QTcpSocket* s = srvr->nextPendingConnection();
            TcpChannel* ch = new TcpChannel(s, this);
            QString url = QString("tcp+aln://%1:%2").arg(srvr->serverAddress().toString()).arg(srvr->serverPort());

            // Check if this connection is already known
            KnownConnectionItem* existing = findKnownConnection(url);
            if (existing) {
                // Update existing connection
                existing->setStatus(Connected);
                existing->setChannel(ch);
            } else {
                // Add new connection
                KnownConnectionItem* newItem = new KnownConnectionItem(url, Connected, ch);
                knownConnections.append(newItem);
            }

            connect(ch, SIGNAL(closing(Channel*)), this, SLOT(onChannelClosing(Channel*)), Qt::QueuedConnection);
            alnRouter->addChannel(ch);
            updateKnownConnectionsTable();
            qInfo() << "Accepting" << s->peerAddress().toString() << "via" << url;
        }
    }
}

void MainWindow::onStartListenButtonClicked() {
    // Get default broadcast address
    QString defaultBroadcastAddr;
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);

    foreach(QNetworkInterface iFace, allInterfaces) {
        QList<QNetworkAddressEntry> entries = iFace.addressEntries();
        foreach(QNetworkAddressEntry entry, entries) {
            QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
                if (iFace.type() == QNetworkInterface::InterfaceType::Ethernet ||
                    iFace.type() == QNetworkInterface::InterfaceType::Wifi) {
                    defaultBroadcastAddr = entry.broadcast().toString();
                    break;
                }
            }
        }
        if (!defaultBroadcastAddr.isEmpty()) break;
    }

    // Show dialog
    StartListenDialog* dialog = new StartListenDialog(defaultBroadcastAddr, 8082, this);
    if (dialog->exec() == QDialog::Accepted) {
        QString addr = dialog->broadcastAddress();
        int port = dialog->port();
        QString key = QString("%1:%2").arg(addr).arg(port);

        // Check if already listening on this address:port
        foreach(DiscoveryListenerItem* item, discoveryListeners) {
            if (item->address() == addr && item->port() == port) {
                qWarning() << "Already listening on" << key;
                dialog->deleteLater();
                return;
            }
        }

        // Create and bind UDP socket
        QUdpSocket* udpSocket = new QUdpSocket(this);
        connect(udpSocket, SIGNAL(readyRead()), this, SLOT(onUdpBroadcastRx()));

        if (udpSocket->bind(port, QUdpSocket::ShareAddress)) {
            // Create listener item
            DiscoveryListenerItem* listenerItem = new DiscoveryListenerItem(addr, port, udpSocket);
            discoveryListeners.append(listenerItem);

            // Also add to udpSockets map for compatibility
            udpSockets.insert(key, udpSocket);

            // Update the model
            QItemSelectionModel *oldSelectionModel = ui->discoveryListenersTableView->selectionModel();
            ui->discoveryListenersTableView->setModel(new DiscoveryListenerItemModel(discoveryListeners, this));
            delete oldSelectionModel;

            ui->discoveryListenersTableView->horizontalHeader()->setSectionResizeMode(DiscoveryListenerColumn::ListenerAddress, QHeaderView::Stretch);
            ui->discoveryListenersTableView->horizontalHeader()->setSectionResizeMode(DiscoveryListenerColumn::ListenerPort, QHeaderView::ResizeToContents);

            // Reconnect selection handler after model change
            connect(ui->discoveryListenersTableView->selectionModel(),
                    SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                    this, SLOT(onDiscoveryListenerSelectionChanged()));

            qInfo() << "Started listening for broadcasts on" << addr << ":" << port;
        } else {
            qWarning() << "Failed to bind to port" << port;
            delete udpSocket;
        }
    }

    dialog->deleteLater();
}

void MainWindow::onDiscoveryListenerSelectionChanged() {
    // Enable Stop button only when a row is selected
    QModelIndexList selectedRows = ui->discoveryListenersTableView->selectionModel()->selectedRows();
    ui->stopListenButton->setEnabled(!selectedRows.isEmpty());
}

void MainWindow::onStopListenButtonClicked() {
    QModelIndexList selectedRows = ui->discoveryListenersTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    int row = selectedRows.first().row();
    if (row < 0 || row >= discoveryListeners.size()) {
        qWarning() << "Invalid row selected:" << row;
        return;
    }

    DiscoveryListenerItem* item = discoveryListeners.at(row);
    QString addr = item->address();
    int port = item->port();
    QString key = QString("%1:%2").arg(addr).arg(port);

    // Disconnect selection handler before model update
    disconnect(ui->discoveryListenersTableView->selectionModel(),
               SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
               this, SLOT(onDiscoveryListenerSelectionChanged()));

    // Remove from udpSockets map
    udpSockets.remove(key);

    // Remove from listeners list (destructor closes socket)
    discoveryListeners.removeAt(row);
    delete item;

    // Update the model
    QItemSelectionModel *oldSelectionModel = ui->discoveryListenersTableView->selectionModel();
    ui->discoveryListenersTableView->setModel(new DiscoveryListenerItemModel(discoveryListeners, this));
    delete oldSelectionModel;

    ui->discoveryListenersTableView->horizontalHeader()->setSectionResizeMode(DiscoveryListenerColumn::ListenerAddress, QHeaderView::Stretch);
    ui->discoveryListenersTableView->horizontalHeader()->setSectionResizeMode(DiscoveryListenerColumn::ListenerPort, QHeaderView::ResizeToContents);

    // Reconnect selection handler after model change
    connect(ui->discoveryListenersTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onDiscoveryListenerSelectionChanged()));

    // Disable the stop button since selection is now cleared
    ui->stopListenButton->setEnabled(false);

    qInfo() << "Stopped listening for broadcasts on" << addr << ":" << port;
}

void MainWindow::onStartAdvertisementButtonClicked() {
    // Build list of available connection strings
    QStringList connectionStrings;
    foreach(NetworkInterfaceItem* item, interfaces) {
        QString connString = QString("tcp+aln://%1:%2").arg(item->listenAddress()).arg(item->listenPort());
        connectionStrings.append(connString);
    }

    // Get default broadcast address
    QString defaultBroadcastAddr;
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);

    foreach(QNetworkInterface iFace, allInterfaces) {
        QList<QNetworkAddressEntry> entries = iFace.addressEntries();
        foreach(QNetworkAddressEntry entry, entries) {
            QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
                if (iFace.type() == QNetworkInterface::InterfaceType::Ethernet ||
                    iFace.type() == QNetworkInterface::InterfaceType::Wifi) {
                    defaultBroadcastAddr = entry.broadcast().toString();
                    break;
                }
            }
        }
        if (!defaultBroadcastAddr.isEmpty()) break;
    }

    int defaultBroadcastPort = 8082;

    // Show dialog with defaults
    StartAdvertisementDialog* dialog = new StartAdvertisementDialog(connectionStrings, defaultBroadcastAddr, defaultBroadcastPort, this);
    if (dialog->exec() == QDialog::Accepted) {
        QString message = dialog->message();
        int periodMs = dialog->broadcastPeriodMs();
        QString broadcastAddr = dialog->broadcastAddress();
        int broadcastPort = dialog->broadcastPort();

        // Create and start advertiser thread with period
        AdvertiserThread* thread = new AdvertiserThread(message, broadcastAddr, broadcastPort, periodMs);
        thread->start(QThread::LowestPriority);

        // Create advertisement item
        AdvertisementItem* adItem = new AdvertisementItem(message, periodMs, broadcastAddr, broadcastPort, thread);
        advertisements.append(adItem);

        // Update the model
        QItemSelectionModel *oldSelectionModel = ui->advertisementsTableView->selectionModel();
        ui->advertisementsTableView->setModel(new AdvertisementItemModel(advertisements, this));
        if (oldSelectionModel) delete oldSelectionModel;

        ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdMessage, QHeaderView::Stretch);
        ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdPeriod, QHeaderView::ResizeToContents);
        ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdStarted, QHeaderView::ResizeToContents);

        // Reconnect selection handler after model change
        connect(ui->advertisementsTableView->selectionModel(),
                SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(onAdvertisementSelectionChanged()));

        qInfo() << "Started advertisement:" << message << "to" << broadcastAddr << ":" << broadcastPort << "every" << (periodMs/1000) << "seconds";
    }

    dialog->deleteLater();
}

void MainWindow::onAdvertisementSelectionChanged() {
    // Enable Stop button only when a row is selected
    QModelIndexList selectedRows = ui->advertisementsTableView->selectionModel()->selectedRows();
    ui->stopAdvertisementButton->setEnabled(!selectedRows.isEmpty());
}

void MainWindow::onStopAdvertisementButtonClicked() {
    QModelIndexList selectedRows = ui->advertisementsTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    int row = selectedRows.first().row();
    if (row < 0 || row >= advertisements.size()) {
        qWarning() << "Invalid row selected:" << row;
        return;
    }

    AdvertisementItem* item = advertisements.at(row);
    QString message = item->message();

    // Disconnect selection handler before model update to prevent signals during cleanup
    disconnect(ui->advertisementsTableView->selectionModel(),
               SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
               this, SLOT(onAdvertisementSelectionChanged()));

    // Remove from advertisements list (destructor stops thread)
    advertisements.removeAt(row);
    delete item;

    // Update the model
    QItemSelectionModel *oldSelectionModel = ui->advertisementsTableView->selectionModel();
    ui->advertisementsTableView->setModel(new AdvertisementItemModel(advertisements, this));
    delete oldSelectionModel;

    ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdMessage, QHeaderView::Stretch);
    ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdPeriod, QHeaderView::ResizeToContents);
    ui->advertisementsTableView->horizontalHeader()->setSectionResizeMode(AdvertisementColumn::AdStarted, QHeaderView::ResizeToContents);

    // Reconnect selection handler after model change
    connect(ui->advertisementsTableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onAdvertisementSelectionChanged()));

    // Disable the stop button since selection is now cleared
    ui->stopAdvertisementButton->setEnabled(false);

    qInfo() << "Stopped advertisement:" << message;
}

void MainWindow::onUdpBroadcastRx() {
    foreach(QUdpSocket* socket, udpSockets) {
        QByteArray datagram;
        while (socket->hasPendingDatagrams()) {
            datagram.resize(int(socket->pendingDatagramSize()));
            socket->readDatagram(datagram.data(), datagram.size());
            QString url = QString::fromUtf8(datagram.constData());

            // Check if this connection is already known
            KnownConnectionItem* existing = findKnownConnection(url);
            if (!existing) {
                // Add new discovered connection as disconnected
                KnownConnectionItem* newItem = new KnownConnectionItem(url, Disconnected, nullptr);
                knownConnections.append(newItem);
                updateKnownConnectionsTable();
                qInfo() << "Discovered connection via broadcast:" << url;
            }

            qDebug() << "Broadcast received:" << url;
        }
    }
}


void MainWindow::onNetStateChanged() {

    QMap<QString, QStringList> nodeMap = alnRouter->nodeServices();

    // clear QButtonGroup
    foreach(QAbstractButton* button, serviceButtonGroup.buttons()) {
        serviceButtonGroup.removeButton(button);
    }
    serviceButtonIdMap.clear();

    QVBoxLayout* layout = new QVBoxLayout();
    foreach(QString nodeAddress, nodeMap.keys()) {
        QFrame* serviceRow = new QFrame;
        serviceRow->setLineWidth(1);
        serviceRow->setFrameStyle(QFrame::Box);

        serviceRow->setLayout(new QVBoxLayout);
        serviceRow->layout()->addWidget(new QLabel(nodeAddress));
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(5);
        buttonLayout->setContentsMargins(0, 0, 0, 0);

        QStringList services = nodeMap.value(nodeAddress);
        if (services.length() == 0) {
            buttonLayout->addWidget(new QLabel("no services"));
        } else {
            foreach(QString service, services) {
                QPushButton* serviceButton = new QPushButton(service);
                int buttonID = serviceButtonIdMap.size();
                serviceButtonIdMap.insert(buttonID, QPair<QString, QString>(nodeAddress, service));
                serviceButtonGroup.addButton(serviceButton, buttonID);
                buttonLayout->addWidget(serviceButton);
            }
            buttonLayout->addStretch(1);
        }

        QWidget* buttonWidget = new QWidget;
        buttonWidget->setLayout(buttonLayout);
        serviceRow->layout()->addWidget(buttonWidget);
        serviceRow->layout()->setSpacing(5);
        serviceRow->layout()->setContentsMargins(5, 5, 5, 0);

        layout->addWidget(serviceRow);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
    }

    layout->addStretch();
    QWidget* widget = new QWidget();
    if (ui->scrollArea->widget()) {
        ui->scrollArea->takeWidget()->deleteLater();
    }
    widget->setLayout(layout);
    ui->scrollArea->setWidget(widget);
    widget->show();
}

void MainWindow::onServiceButtonClicked(int id) {
    QPair<QString, QString> data = serviceButtonIdMap.value(id);
    QString nodeAddress = data.first;
    QString service = data.second;
    PacketSendDialog* dialog = new PacketSendDialog(alnRouter, this);
    dialog->setDest(nodeAddress);
    dialog->setService(service);
    dialog->show();
}

void MainWindow::btDiscoveryCheckboxChanged(bool enabled) {
    if (enabled) {
        qInfo() << "startint BT discovery";
        btDiscoveryAgent->start();
    } else {
        qInfo() << "stopping BT discovery";
        btDiscoveryAgent->stop();
    }
}

void MainWindow::deviceDiscovered(QBluetoothDeviceInfo device) {
    foreach(QBluetoothUuid serviceUUID, device.serviceUuids()) {
        qDebug() << "inspecting " <<serviceUUID.toString(QBluetoothUuid::WithoutBraces);
        if( serviceUuidToNameMap.contains(serviceUUID.toString(QBluetoothUuid::WithoutBraces))) {
            qDebug() << "discovered BT device: " << device.name();
            auto pair = QPair<QBluetoothDeviceInfo, QBluetoothUuid>(device, serviceUUID);
            btAddressToDeviceInfoMap.insert(device.address().toString(), pair);
            btDiscoveryStateChanged();
            break;
        }
    }
}

void MainWindow::btDiscoveryStateChanged() {
    if (ui->btDiscoveryScrollArea->widget()) { // remove previous layout
        ui->btDiscoveryScrollArea->takeWidget()->deleteLater();
    }
    foreach(QAbstractButton* button, btConnectButtonGroup.buttons()) {
        btConnectButtonGroup.removeButton(button);
    }

    QVBoxLayout* layout = new QVBoxLayout();
    foreach(QString btAddress, btAddressToDeviceInfoMap.keys()) {
        QFrame* serviceRow = new QFrame;
        serviceRow->setLineWidth(1);
        serviceRow->setFrameStyle(QFrame::Box);
        serviceRow->setLayout(new QHBoxLayout);
        serviceRow->layout()->addWidget(new QLabel(btAddress));

        QPair<QBluetoothDeviceInfo, QBluetoothUuid> pair = btAddressToDeviceInfoMap.value(btAddress);
        QString deviceName = pair.first.name();
        serviceRow->layout()->addWidget(new QLabel(deviceName));

        // TODO if not connected
        QPushButton* connectButton = new QPushButton("Connect");
        int id = btConnectIdToAddressMap.size();
        btConnectIdToAddressMap.insert(id, btAddress);
        btConnectButtonGroup.addButton(connectButton, id);
        serviceRow->layout()->addWidget(connectButton);

        layout->addWidget(serviceRow);
    }
    layout->addStretch();

    QWidget* widget = new QWidget();
    widget->setLayout(layout);
    ui->btDiscoveryScrollArea->setWidget(widget);
    widget->show();
}

void MainWindow::onBtConnectButtonClicked(int id) {
    QString btAddress = btConnectIdToAddressMap.value(id);
    QPair<QBluetoothDeviceInfo, QBluetoothUuid> pair = btAddressToDeviceInfoMap.value(btAddress);
    QBluetoothUuid serviceUUID = pair.second;

    QBluetoothSocket* socket;
    if (btAddressSocketMap.contains(btAddress)){
        socket = btAddressSocketMap.value(btAddress);
    } else {
        socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
        btAddressSocketMap.insert(btAddress, socket);
    }
    if (socket->isOpen()) {
        qInfo()<< "closing BT socket " << serviceUUID;
        socket->close();
    } else {
        qInfo()<< "connecting to BT socket " << serviceUUID;
        socket->connectToService(QBluetoothAddress(btAddress), serviceUUID);
    }
}
