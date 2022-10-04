#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "advertiserthread.h"
#include "connectionitemmodel.h"
#include "networkinterfacesitemmodel.h"

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTcpServer>

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
    // TODO client sockets
    QMap<QString, AdvertiserThread*> urlAdvertisers;
    QList<ConnectionItem*> connectionItems;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void selfTest();
    void populateNetworkInterfaces();
    Ui::MainWindow *ui;
    bool testFlag = false;

public slots:
    void onAddChannelButtonClicked();
    void onChannelClosing(Channel*);
    void onConnectRequest(QString url);
    void onListenRequest(QString interface, int port, bool enable);
    void onTcpListenPending();

    void onBroadcastAdvertRequest(QString url, QString addr, int port, bool enable);
    void onBroadcastListenRequest(QString addr, int port, bool enable); // TODO


signals:
    void connectionAdded(QString url);
};
#endif // MAINWINDOW_H
