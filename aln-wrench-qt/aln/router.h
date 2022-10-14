#ifndef ROUTER_H
#define ROUTER_H

#include <QDate>
#include <QDate>
#include <QMutex>
#include <QObject>
#include "channel.h"
#include "quuid.h"


class RemoteNodeInfo {
public:
    QString address; // target addres to communicate with
    QString nextHop; // next routing node for address
    Channel* channel; // specific channel that is hosting next hop
    short cost; // cost of using this route, generally a hop count
    QString err;
};

class ServiceNodeInfo {
public:
    QString service; // name of the service
    QString address; // host of the service
    QString nextHop; // next routing node for address
    short load; // remote service load factor
    QString err; // parser error
};

class NodeLoad {
public:
    short load;
    QDate lastSeen;
};

class PacketHandler {
public:
    virtual void onPacket(Packet*) = 0;
};


class Router : public QObject
{
    Q_OBJECT
    QMutex mMutex;
    QString mAddress = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);

    QHash<short, PacketHandler*> contextHandlerMap;
    QHash<QString, PacketHandler*> serviceHandlerMap;

    // map[address]RemoteNode
    QMap<QString, RemoteNodeInfo*> remoteNodeMap;

    // map[service][address]NodeLoad
    QMap<QString, QMap<QString, NodeLoad*>> serviceLoadMap;

    QVector<Channel*> channels;
    QMap<QString, QVector<Packet*>> serviceQueue;

public:
    Router(QString address = QString());
    QString address() { return mAddress; }

    void addChannel(Channel*);
    void removeChannel(Channel*);

    QString selectServiceAddress(QString); // returns the least load node with service
    QString send(Packet* p);
    void registerService(QString service, PacketHandler* handler);
    void unregisterService(QString service);
    short registerContextHandler(PacketHandler*);
    void releaseContext(short);

    QMap<QString, QStringList> nodeServices();

public slots:
    void onPacket(Channel*, Packet*);
    void onChannelClose(Channel*);

signals:
    void channelsChanged();
    void netStateChanged();

private:
    void handleNetState(Channel*, Packet*);

    Packet* composeNetRouteShare(QString address, short cost);
    RemoteNodeInfo parseNetRouteShare(Packet* packet);
    Packet* composeNetServiceShare(QString address, QString service, short load);
    ServiceNodeInfo parseNetServiceShare(Packet* packet);
    Packet* composeNetQuery();

    QList<Packet*> exportRouteTable();
    QList<Packet*> exportServiceTable();
    void shareNetState();

    void removeAddress(QString);
};


#endif // ROUTER_H
