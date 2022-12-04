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
    connect(ui->addChannelButton, SIGNAL(clicked()),
            this, SLOT(onAddChannelButtonClicked()));

    alnRouter = new Router();
    PacketSignaler* signaler = new PacketSignaler();
    connect(signaler, SIGNAL(packetReceived(Packet*)), this, SLOT(logServicePacketHandler(Packet*)));
    alnRouter->registerService("log", signaler);

    connect(alnRouter, SIGNAL(netStateChanged()),
            this, SLOT(onNetStateChanged()));
    selfTest();

    populateNetworkInterfaces();
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));

    connect(&serviceButtonGroup, SIGNAL(idClicked(int)),
            this, SLOT(onServiceButtonClicked(int)));

    // network host interfaces model
    connect(niim, SIGNAL(listenRequest(QString,QString,short,bool)),
            this, SLOT(onListenRequest(QString,QString,short,bool)));

    // network advertising panel
    if (interfaces.isEmpty()) {
        ui->bcastInterfaceLineEdit->setText("no network interfaces found");
        ui->bcastInterfaceLineEdit->setEnabled(false);
    } else {
        QString interfaceName = interfaces.at(0)->broadcastAddress();
        ui->bcastInterfaceLineEdit->setText(interfaceName);
        ui->networkDiscoveryLineEdit->setText(interfaceName);
    }

    connect(ui->netBroadcastEnableCheckbox, SIGNAL(stateChanged(int)),
            this, SLOT(onBroadcastAdvertRequest(int)));

    connect(ui->broadcastListenCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(onBroadcastListenRequest(int)));

    // bluetooth discovery
    btDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(btDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    connect(ui->btDiscoveryGroupBox, SIGNAL(toggled(bool)),
            this, SLOT(btDiscoveryCheckboxChanged(bool)));
    connect(&btConnectButtonGroup, SIGNAL(idClicked(int)),
            this, SLOT(onBtConnectButtonClicked(int)));

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

void MainWindow::addLogLine(QString msg) {
    while (debugBufferList.size() > 20)
        debugBufferList.removeFirst();
    debugBufferList.append(msg);
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
            emit logLineReady(QString("%0 WARNING %1 (%2:%3, %4)").arg(QDateTime::currentDateTime().toString( Qt::ISODateWithMs)).arg(msg));
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        if (ui->logErrorCheckBox->isChecked())
            emit logLineReady(QString("%0 CRITICAL %1 (%2:%3, %4)").arg(QDateTime::currentDateTime().toString( Qt::ISODateWithMs)).arg(msg));
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
    QStringList networkInterfaces, types, bcastAddresses;
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    foreach(QNetworkInterface iFace, allInterfaces) {
        QList<QNetworkAddressEntry> entries = iFace.addressEntries();
            foreach(QNetworkAddressEntry entry, entries) {
                QHostAddress address = entry.ip();
                if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
                    switch(iFace.type()){
                    case QNetworkInterface::InterfaceType::Ethernet:
                        networkInterfaces << address.toString();
                        types << "eth";
                        bcastAddresses << entry.broadcast().toString();
                        break;
                    case QNetworkInterface::InterfaceType::Wifi:
                        networkInterfaces << address.toString();
                        types << "wifi";
                        bcastAddresses << entry.broadcast().toString();
                        break;
                    default:
                        break;
                    }
                }
            }
    }

    for(int i = 0; i < networkInterfaces.count(); i++) {
        interfaces << new NetworkInterfaceItem(types.at(i), networkInterfaces.at(i), 8081, bcastAddresses.at(i));
    }

    niim = new NetworkInterfacesItemModel(interfaces, this);
    ui->networkInterfacesTableView->setModel(niim);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Type, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Scheme, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Address, QHeaderView::Stretch);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::ListenPort, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Listen, QHeaderView::ResizeToContents);
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
}

void MainWindow::onConnectRequest(QString urlString) {
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
    connectionItems.append(new ConnectionItem(channel, urlString));
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));
    connect(channel, SIGNAL(closing(Channel*)), this, SLOT(onChannelClosing(Channel*)), Qt::QueuedConnection);
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

    if (ui->netAdvertiseContent->isEnabled())
        ui->netAdvertiseContent->setText(QString("%1://%2:%3").arg(scheme).arg(interface).arg(port));
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
    qInfo() << "Enable BroadcastListen:" << (enable ? "true" : "false");
    QString addr = ui->networkDiscoveryLineEdit->text();
    int port = ui->networkDiscoveryPortSpinBox->value();
    QString key = QString("%1:%2").arg(addr).arg(port);

    if (urlAdvertisers.contains(key)) {
        qInfo() << "cannot listen on advertising port";
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
    QVBoxLayout* layout = new QVBoxLayout();
    foreach(QString advert, udpAdverts.keys()) {
        QHBoxLayout* rowLayout = new QHBoxLayout();
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

void MainWindow::onNetStateChanged() {

    QMap<QString, QStringList> nodeMap = alnRouter->nodeServices();

    // clear QButtonGroup
    foreach(QAbstractButton* button, serviceButtonGroup.buttons()) {
        serviceButtonGroup.removeButton(button);
    }
    buttonIdMap.clear();

    QVBoxLayout* layout = new QVBoxLayout();
    foreach(QString nodeAddress, nodeMap.keys()) {
        QFrame* serviceRow = new QFrame;
        serviceRow->setLineWidth(1);
        serviceRow->setFrameStyle(QFrame::Box);

        serviceRow->setLayout(new QVBoxLayout);
        serviceRow->layout()->addWidget(new QLabel(nodeAddress));
        QHBoxLayout* buttonLayout = new QHBoxLayout;

        QStringList services = nodeMap.value(nodeAddress);
        if (services.length() == 0) {
            buttonLayout->addWidget(new QLabel("no services"));
        } else {
            foreach(QString service, services) {
                QPushButton* serviceButton = new QPushButton(service);
                int buttonID = buttonIdMap.size();
                buttonIdMap.insert(buttonID, QPair<QString, QString>(nodeAddress, service));
                serviceButtonGroup.addButton(serviceButton, buttonID);
                buttonLayout->addWidget(serviceButton);
            }
        }

        QWidget* buttonWidget = new QWidget;
        buttonWidget->setLayout(buttonLayout);
        serviceRow->layout()->addWidget(buttonWidget);
        layout->addWidget(serviceRow);
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
    QPair<QString, QString> data = buttonIdMap.value(id);
    QString nodeAddress = data.first;
    QString service = data.second;
    // TODO create dialog
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
            qInfo() << "discovered bt device: " << device.name();
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
