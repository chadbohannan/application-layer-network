#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "advertiserthread.h"
#include "advertisementitemmodel.h"
#include "networkinterfacesitemmodel.h"
#include "discoverylisteneritemmodel.h"
#include "knownconnectionitemmodel.h"
#include "openportdialog.h"
#include "startadvertisementdialog.h"
#include "startlistendialog.h"

#include <QButtonGroup>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QTcpServer>
#include <QUdpSocket>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QList>

#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>

#include <aln/packet.h>
#include <aln/channel.h>
#include <aln/tcpchannel.h>

#include <aln/router.h>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Router* alnRouter;

    NetworkInterfacesItemModel *niim;
    QMap<QString, QTcpServer*> tcpServers;
    QMap<QString, QUdpSocket*> udpSockets;
    QMap<QString, bool> udpAdverts;
    // TODO client sockets

    QList<NetworkInterfaceItem*> interfaces;
    QList<AdvertisementItem*> advertisements;
    QList<DiscoveryListenerItem*> discoveryListeners;
    QList<KnownConnectionItem*> knownConnections;

    QButtonGroup serviceButtonGroup;
    QMap<int, QPair<QString, QString>> serviceButtonIdMap;

    QBluetoothDeviceDiscoveryAgent* btDiscoveryAgent;
    QMap<QString, QString> serviceUuidToNameMap;
    QMap<QString, QPair<QBluetoothDeviceInfo,QBluetoothUuid>> btAddressToDeviceInfoMap;
    QButtonGroup btConnectButtonGroup;
    QMap<int, QString> btConnectIdToAddressMap;
    QMap<QString, QBluetoothSocket*> btAddressSocketMap;

    QList<QString> debugBufferList;
    QList<QString> logServiceBufferList;

    Ui::MainWindow *ui;
    bool testFlag = false;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void init();

private:
    void selfTest();
    void populateNetworkInterfaces();
    void updateKnownConnectionsTable();
    KnownConnectionItem* findKnownConnection(QString url);

public slots:
    void addLogLine(QString msg);
    void clearLog();
    void onDebugMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void onOpenPortButtonClicked();
    void onClosePortButtonClicked();
    void onNetworkInterfaceSelectionChanged();

    void onStartAdvertisementButtonClicked();
    void onStopAdvertisementButtonClicked();
    void onAdvertisementSelectionChanged();

    void onAddConnectionButtonClicked();
    void onConnectButtonClicked();
    void onDisconnectButtonClicked();
    void onKnownConnectionSelectionChanged();
    void onChannelClosing(Channel*);
    void onListenRequest(QString scheme, QString interface, short port, bool enable);
    void onTcpListenPending();
    void onUdpBroadcastRx();

    void onStartListenButtonClicked();
    void onStopListenButtonClicked();
    void onDiscoveryListenerSelectionChanged();

    void btDiscoveryCheckboxChanged(bool);
    void deviceDiscovered(QBluetoothDeviceInfo);
    void btDiscoveryStateChanged();
    void onBtConnectButtonClicked(int);

    void onNetStateChanged();
    void logServicePacketHandler(Packet* packet);
    void echoServicePacketHandler(Packet* packet);

    void onServiceButtonClicked(int id);

signals:
    void logLineReady(QString);
    void connectionAdded(QString url);
};
#endif // MAINWINDOW_H
