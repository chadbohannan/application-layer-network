#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "advertiserthread.h"
#include "connectionitemmodel.h"
#include "networkinterfacesitemmodel.h"

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
    // TODO client sockets

    QList<NetworkInterfaceItem*> interfaces;
    QLineEdit* bcastInterfaceLineEdit;
    QSpinBox* bcastPortSpinbox;
    QCheckBox* netBroadcastEnableCheckbox;
    QLineEdit* netAdvertiseContent;
    QMap<QString, AdvertiserThread*> urlAdvertisers;
    QList<ConnectionItem*> connectionItems;

    QButtonGroup serviceButtonGroup;
    QMap<int, QPair<QString, QString>> buttonIdMap;

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


public slots:
    void addLogLine(QString msg);
    void onDebugMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    void onAddChannelButtonClicked();
    void onChannelClosing(Channel*);
    void onConnectRequest(QString url);
    void onListenRequest(QString scheme, QString interface, short port, bool enable);
    void onTcpListenPending();
    void onUdpBroadcastRx();

    void onBroadcastAdvertRequest(int checkState);
    void onBroadcastListenRequest(QString addr, short port, bool enable); // TODO

    void btDiscoveryCheckboxChanged(bool);
    void deviceDiscovered(QBluetoothDeviceInfo);
    void btDiscoveryStateChanged();
    void onBtConnectButtonClicked(int);

    void onNetStateChanged();
    void logServicePacketHandler(Packet* packet);

    void onServiceButtonClicked(int id);
signals:
    void logLineReady(QString);
    void connectionAdded(QString url);
};
#endif // MAINWINDOW_H
