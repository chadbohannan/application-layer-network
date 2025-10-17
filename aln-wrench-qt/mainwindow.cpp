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
    foreach(AdvertiserThread* at, urlAdvertisers.values()) {
        at->stop();
    }
    delete ui;
}

void MainWindow::init() {
    connect(ui->openPortButton, SIGNAL(clicked()),
            this, SLOT(onOpenPortButtonClicked()));
    connect(ui->closePortButton, SIGNAL(clicked()),
            this, SLOT(onClosePortButtonClicked()));
    connect(ui->addChannelButton, SIGNAL(clicked()),
            this, SLOT(onAddChannelButtonClicked()));

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
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));

    connect(&serviceButtonGroup, SIGNAL(idClicked(int)),
            this, SLOT(onServiceButtonClicked(int)));

    // network advertising panel - discover first available interface for defaults
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    QString defaultBroadcastAddr;

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

    if (defaultBroadcastAddr.isEmpty()) {
        ui->bcastInterfaceLineEdit->setText("no network interfaces found");
        ui->bcastInterfaceLineEdit->setEnabled(false);
    } else {
        ui->bcastInterfaceLineEdit->setText(defaultBroadcastAddr);
        ui->networkDiscoveryLineEdit->setText(defaultBroadcastAddr);
    }

    connect(ui->netBroadcastEnableCheckbox, SIGNAL(stateChanged(int)),
            this, SLOT(onBroadcastAdvertRequest(int)));

    connect(ui->broadcastListenCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(onBroadcastListenRequest(int)));

    connect(&connectToHostButtonGroup, SIGNAL(idClicked(int)),
            this, SLOT(onNetworkHostConnectButtonClicked(int)));

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

            // Update broadcast advertisement text if enabled
            if (ui->netAdvertiseContent->isEnabled()) {
                ui->netAdvertiseContent->setText(QString("tcp+aln://%1:%2").arg(interface).arg(port));
                ui->bcastInterfaceLineEdit->setText(broadcastAddr);
            }
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

void MainWindow::onAddChannelButtonClicked() {
    AddChannelDialog* dialog = new AddChannelDialog(this);
    connect(dialog, SIGNAL(connectRequest(QString)), this, SLOT(onConnectRequest(QString)));
    dialog->show();
}

void MainWindow::onChannelClosing(Channel* ch)
{
    for(int i = 0; i < connectionItems.length(); i++) {
        if (connectionItems.at(i)->channel() == ch) {
            connectionItems.removeAt(i);
            break;
        }
    }
    QItemSelectionModel *m = ui->channelsListView->selectionModel();
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));
    if (m) delete m;
    onNetworkDiscoveryChanged();
}

void MainWindow::onConnectRequest(QString urlString) {
    QUrl url = QUrl::fromEncoded(urlString.toUtf8());
    foreach(ConnectionItem* item, connectionItems) {
        if (item->info() == urlString) {
            qInfo() << "already connected to " << urlString;
            return;
        }
    }
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
    connectionItems.append(new ConnectionItem(channel, urlString));
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));
    connect(channel, SIGNAL(closing(Channel*)), this, SLOT(onChannelClosing(Channel*)), Qt::QueuedConnection);
}

void MainWindow::onDisconnectRequest(QString url) {
    foreach(ConnectionItem *item, connectionItems) {
        if (item->info() == url)  {
            item->channel()->disconnect();
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

    if (ui->netAdvertiseContent->isEnabled()) {
        ui->netAdvertiseContent->setText(QString("%1://%2:%3").arg(scheme).arg(interface).arg(port));
        foreach(NetworkInterfaceItem* item, interfaces){
            if (item->listenAddress() == interface) {
                ui->bcastInterfaceLineEdit->setText(item->broadcastAddress());
            }
        };
    }
}

void MainWindow::onTcpListenPending() {
    foreach (QTcpServer* srvr, tcpServers) {
        while (srvr->hasPendingConnections()) {
            QTcpSocket* s = srvr->nextPendingConnection();
            TcpChannel* ch = new TcpChannel(s, this);
            QString url = QString("tcp+aln://%1:%2").arg(srvr->serverAddress().toString()).arg(srvr->serverPort());
            connectionItems.append(new ConnectionItem(ch, url));
            connect(ch, SIGNAL(closing(Channel*)), this, SLOT(onChannelClosing(Channel*)), Qt::QueuedConnection);
            alnRouter->addChannel(ch);
            qInfo() << "accepting " <<s->peerAddress().toString() << " via" << url;
        }
    }
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));
}

void MainWindow::onBroadcastListenRequest(int checkState) {
    bool enable = checkState == Qt::Checked;
    QString addr = ui->networkDiscoveryLineEdit->text();
    int port = ui->networkDiscoveryPortSpinBox->value();
    QString key = QString("%1:%2").arg(addr).arg(port);

    if (urlAdvertisers.contains(key)) {
        qWarning() << "cannot listen on advertising port";
        return;
    }

    QUdpSocket* udpSocket;
    if (udpSockets.contains(key)) {
        udpSocket = udpSockets.value(key);
    } else {
        udpSocket = new QUdpSocket(this);
        udpSockets.insert(key, udpSocket);
        connect(udpSocket, SIGNAL(readyRead()), this, SLOT(onUdpBroadcastRx()));
    }
    if (enable && !udpSocket->isOpen()) {
        udpSocket->bind(port, QUdpSocket::ShareAddress);
    }
    if (!enable){
        if (udpSocket->isOpen())
            udpSocket->disconnectFromHost();
        if (udpSockets.contains(key))
            udpSockets.remove(key);
        delete udpSocket;
    }
}

void MainWindow::onUdpBroadcastRx() {
    foreach(QUdpSocket* socket, udpSockets) {
        QByteArray datagram;
        while (socket->hasPendingDatagrams()) {
            datagram.resize(int(socket->pendingDatagramSize()));
            socket->readDatagram(datagram.data(), datagram.size());
            if (!udpAdverts.contains(datagram.constData())) {
                udpAdverts[datagram.constData()] = true;
                onNetworkDiscoveryChanged();
            }
            qDebug() << "bcast recvd:" << datagram.constData();
        }
    }
}

void MainWindow::onNetworkDiscoveryChanged() {
    // clear QButtonGroup
    foreach(QAbstractButton* button, connectToHostButtonGroup.buttons()) {
        connectToHostButtonGroup.removeButton(button);
    }
    connectToHostButtonIdMap.clear();

    QVBoxLayout* layout = new QVBoxLayout();
    foreach(QString advert, udpAdverts.keys()) {
        QHBoxLayout* rowLayout = new QHBoxLayout();
        QPushButton* connectButton = new QPushButton(hasConnection(advert) ? "disconnect" : "connect");

        int buttonID = connectToHostButtonIdMap.size();
        connectToHostButtonIdMap.insert(buttonID, advert);
        connectToHostButtonGroup.addButton(connectButton, buttonID);

        rowLayout->addWidget(connectButton);
        rowLayout->addStretch();
        rowLayout->addWidget(new QLabel(advert));
        layout->addLayout(rowLayout);
    }

    layout->addStretch();
    QWidget* widget = new QWidget();
    if (ui->netDiscoveryScrollArea->widget()) {
        ui->netDiscoveryScrollArea->takeWidget()->deleteLater();
    }
    widget->setLayout(layout);
    ui->netDiscoveryScrollArea->setWidget(widget);
    widget->show();
}

bool MainWindow::hasConnection(QString url) {
    bool connected = false;
    foreach(ConnectionItem *item, connectionItems) {
        if (item->info() == url)  {
            connected = true;
            break;
        }
    }
    return connected;
}

void MainWindow::onNetworkHostConnectButtonClicked(int id) {
    QString url = connectToHostButtonIdMap.value(id);
    if (hasConnection(url)) {
        onDisconnectRequest(url);
    } else {
        onConnectRequest(url);
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

    // update discovery button text
    onNetworkDiscoveryChanged();
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

void MainWindow::onBroadcastAdvertRequest(int checkState) {
    bool enable = checkState == Qt::CheckState::Checked;
    QString addr = ui->bcastInterfaceLineEdit->text();
    int port = ui->bcastPortSpinbox->value();
    QString url = ui->netAdvertiseContent->text();

    QString key = QString("%1:%2").arg(addr).arg(port);
    AdvertiserThread* adThread;
    if (urlAdvertisers.contains(key)) {
        adThread = urlAdvertisers.value(key);
    } else {
        adThread = new AdvertiserThread(url, addr, port);
        urlAdvertisers.insert(key, adThread);
    }
    if (adThread->isRunning() && !enable) {
        qInfo() << "stopping broadcast on " << key;
        adThread->stop();
    } else if (!adThread->isRunning() && enable) {
        qInfo() << "starting broadcast on " << key << " of " << url;
        adThread->setUrl(url);
        adThread->start(QThread::LowestPriority);
    }

    ui->bcastInterfaceLineEdit->setEnabled(!enable);
    ui->bcastPortSpinbox->setEnabled(!enable);
    ui->netAdvertiseContent->setEnabled(!enable);
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
