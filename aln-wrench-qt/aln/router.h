#ifndef ROUTER_H
#define ROUTER_H

#include <QDate>
#include <QDate>
#include <QObject>
#include "channel.h"
#include "quuid.h"


class RemoteNodeInfo {
public:
    QString address; // target addres to communicate with
    QString nextHop; // next routing node for address
    Channel* channel; // specific channel that is hosting next hop
    short cost; // cost of using this route, generally a hop count
};

class NodeLoad {
public:
    short load;
    QDate lastSeen;
};

class Router : public QObject
{
    Q_OBJECT
    QString mAddress = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);

    // map[address]RemoteNode
    QMap<QString, RemoteNodeInfo*> remoteNodeMap;

    // map[service][address]NodeLoad
    QMap<QString, QMap<QString, NodeLoad*>> serviceLoadMap;

    QVector<Channel*> channels;
    QMap<QString, QVector<Packet*>> serviceQueue;

public:
    Router(QString address = QString());
    QString address() { return mAddress; }
};


#endif // ROUTER_H
