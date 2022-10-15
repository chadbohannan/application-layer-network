#include "addchanneldialog.h"
#include "mainwindow.h"
#include "packetsenddialog.h"
#include "ui_mainwindow.h"

#include "aln/packet.h"
#include "aln/parser.h"
#include "aln/ax25frame.h"
#include <aln/localchannel.h>

#include <QNetworkInterface>
#include <QSslSocket>
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->addChannelButton, SIGNAL(clicked()), this, SLOT(onAddChannelButtonClicked()));

    alnRouter = new Router();

    connect(alnRouter, SIGNAL(netStateChanged()), this, SLOT(onNetStateChanged()));
    selfTest();

    populateNetworkInterfaces();
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));

    // TODO connect serviceButtonGroup
    connect(&serviceButtonGroup, SIGNAL(idClicked(int)), this, SLOT(onServiceButtonClicked(int)));

    connect(niim, SIGNAL(listenRequest(QString,int,bool)), this, SLOT(onListenRequest(QString,int,bool)));
    connect(niim, SIGNAL(advertiseRequest(QString, QString, int, bool)), this, SLOT(onBroadcastAdvertRequest(QString,QString,int,bool)));
    connect(niim, SIGNAL(advertiseListenRequest(QString,int,bool)), this, SLOT(onBroadcastListenRequest(QString,int,bool)));

    this->statusBar()->showMessage("Application Layer Network Node Address: " + alnRouter->address());
}

MainWindow::~MainWindow()
{
    foreach(AdvertiserThread* at, urlAdvertisers.values()) {
        at->stop();
    }
    delete ui;
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
    foreach(QNetworkInterface iFace, allInterfaces) {
        QList<QNetworkAddressEntry> entries = iFace.addressEntries();
        if (entries.isEmpty()) continue;
        QNetworkAddressEntry entry = entries.first();
        QHostAddress address = entry.ip();
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

    QList<NetworkInterfaceItem*> interfaces;
    for(int i = 0; i < networkInterfaces.count(); i++) {
        interfaces << new NetworkInterfaceItem(types.at(i), networkInterfaces.at(i), 8081, bcastAddresses.at(i));
    }

    niim = new NetworkInterfacesItemModel(interfaces, this);
    ui->networkInterfacesTableView->setModel(niim);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Type, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Address, QHeaderView::Stretch);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::ListenPort, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Listen, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::BroadcastPort, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::Advertise, QHeaderView::ResizeToContents);
    ui->networkInterfacesTableView->horizontalHeader()->setSectionResizeMode(Column::BroadcastListen, QHeaderView::ResizeToContents);
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

void MainWindow::onListenRequest(QString interface, int port, bool enable) {
    QTcpServer* srvr;
    QString key = QString("%1:%2").arg(interface).arg(port);
    if (tcpServers.contains(key)) {
        srvr = tcpServers.value(key);
    } else {
        srvr = new QTcpServer(this);
        connect(srvr, SIGNAL(newConnection()), this,SLOT(onTcpListenPending()));
        tcpServers.insert(key, srvr);
    }
    if (enable && !srvr->isListening()) {
        srvr->listen(QHostAddress(interface), port);
    } else if (!enable && srvr->isListening()) {
        srvr->close();
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
        }
    }
    ui->channelsListView->setModel(new ConnectionItemModel(connectionItems, this));
}

void MainWindow::onBroadcastListenRequest(QString addr, int port, bool enable) {
    qDebug() << "TODO onBroadcastListenRequest";
}

void MainWindow::onNetStateChanged() {
    QVBoxLayout* layout = new QVBoxLayout();

    QMap<QString, QStringList> nodeMap = alnRouter->nodeServices();

    // clear QButtonGroup
    foreach(QAbstractButton* button, serviceButtonGroup.buttons()) {
        serviceButtonGroup.removeButton(button);
    }
    buttonIdMap.clear();

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
    qDebug() << "onServiceButtonClicked" << nodeAddress << service;
}

void MainWindow::onBroadcastAdvertRequest(QString url, QString addr, int port, bool enable) {
    QString key = QString("%1:%2").arg(addr).arg(port);
    AdvertiserThread* adThread;
    if (urlAdvertisers.contains(key)) {
        adThread = urlAdvertisers.value(key);
    } else {
        adThread = new AdvertiserThread(url, addr, port);
        urlAdvertisers.insert(key, adThread);
    }
    if (adThread->isRunning() && !enable) {
        adThread->stop();
    } else if (!adThread->isRunning() && enable) {
        adThread->start(QThread::LowestPriority);
    }
}
