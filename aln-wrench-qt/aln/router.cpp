#include "router.h"
#include <QRandomGenerator>

Router::Router(QString address) {
    if (address.length() > 0) {
        mAddress = address;
    }
}

QString Router::selectServiceAddress(QString service) {
    if (serviceHandlerMap.contains(service))
        return mAddress;

    short minLoad = 0;
    QString remoteAddress = "";
    if (serviceLoadMap.contains(service)) {
        QMap<QString, NodeLoad*> addressToLoadMap = serviceLoadMap[service];
        foreach (QString address, addressToLoadMap.keys()) {
            NodeLoad* nodeLoad = addressToLoadMap[address];
            if (minLoad == 0 || minLoad > nodeLoad->load) {
                remoteAddress = address;
                minLoad = nodeLoad->load;
            }
        }
    }
    return remoteAddress;
}

QString Router::send(Packet* p) {
    QMutexLocker lock = QMutexLocker(&mMutex);
    if (p->srcAddress.length() == 0) {
        p->srcAddress = mAddress;
    }
    if (p->destAddress.length() == 0 && p->srv.length() > 0) {
        // TODO send packet to all matching services
        p->destAddress = selectServiceAddress(p->srv);
        if (p->destAddress.length() == 0) {
            QList<Packet*> packets;
            if (serviceQueue.contains(p->srv)) {
                packets = serviceQueue[p->srv];
            }
            packets.append(p);
            serviceQueue.insert(p->srv, packets);
            return "service unavailable, packet queued";
        }
    }
    lock.unlock();

    if (p->destAddress == mAddress) {
        PacketHandler* handler;
        if (serviceHandlerMap.contains(p->srv)) {
            handler = serviceHandlerMap[p->srv];
            handler->onPacket(p);
        } else if (contextHandlerMap.contains(p->ctx)) {
            handler = contextHandlerMap[p->ctx];
            handler->onPacket(p);
        } else {
            return QString("service '%1' not registered\n").arg(p->srv);
        }
    } else if (p->nxtAddress.length() == 0 || p->nxtAddress == mAddress) {
        if (remoteNodeMap.contains(p->destAddress)) {
            RemoteNodeInfo* rni = remoteNodeMap[p->destAddress];
            p->nxtAddress = rni->nextHop;
            rni->channel->send(p);
        }
        // TODO detect and broadcast route failure
        return "send failed; no route to " + p->destAddress;
    } else {
        return "packet is unroutable; no action taken";
    }
    return QString();
}

short Router::registerContextHandler(PacketHandler* handler) {
    QMutexLocker lock(&mMutex);
    short newCtx = QRandomGenerator::global()->generate() % ((1 << 16)-1);
    while (contextHandlerMap.contains(newCtx))
        newCtx = QRandomGenerator::global()->generate() % ((1 << 16)-1);
    contextHandlerMap.insert(newCtx, handler);
    return newCtx;
}

void Router::releaseContext(short ctx) {
    QMutexLocker lock(&mMutex);
    contextHandlerMap.remove(ctx);
}

Packet* Router::composeNetRouteShare(QString address, short cost) {
    Packet* p = new Packet();
    p->net = Packet::NetState::ROUTE;
    p->srcAddress = mAddress;
    p->data.clear();
    QBuffer buffer(&p->data);
    buffer.open(QIODevice::Append);
    writeToBuffer(&buffer, (INT08U)address.length());
    writeToBuffer(&buffer, address);
    writeToBuffer(&buffer, (INT16U)cost);
    buffer.close();
    return p;
}

RemoteNodeInfo Router::parseNetRouteShare(Packet* p) {
    RemoteNodeInfo info;
    if (p->net != Packet::NetState::ROUTE) {
        info.err = "parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE";
        return info;
    }
    if (p->data.length() == 0) {
        info.err = "parseNetworkRouteSharePacket: packet.Data is empty";
        return info;
    }

    QByteArray data = p->data;
    INT08U addrSize = data[0];

    if (data.length() != addrSize + 3) {
        info.err = QString("parseNetworkRouteSharePacket: len: %1; exp: %2").arg(data.length()).arg(addrSize + 3);
        return info;
    }

    int offset = 1;

    info.address = data.mid(offset, addrSize);
    offset += addrSize;
    info.cost = readINT16U((INT08U*)data.mid(offset, 2).data());
    info.nextHop = p->srcAddress;

    return info;
}

Packet* Router::composeNetServiceShare(QString address, QString service, short load) {
    Packet* p = new Packet();
    p->net = Packet::NetState::SERVICE;
    p->srcAddress = mAddress;
    p->data.clear();
    QBuffer buffer(&p->data, this);
    buffer.open(QIODevice::Append);
    writeToBuffer(&buffer, (INT08U)address.length());
    writeToBuffer(&buffer, address);
    writeToBuffer(&buffer, (INT08U)service.length());
    writeToBuffer(&buffer, service);
    writeToBuffer(&buffer, (INT16U)load);
    buffer.close();
    return p;
}

ServiceNodeInfo Router::parseNetServiceShare(Packet* p) {
    ServiceNodeInfo info;
    if (p->net != Packet::NetState::SERVICE) {
        info.err = "parseNetworkRouteSharePacket: packet.NetState != NET_SERVICE";
        return info;
    }
    if (p->data.length() == 0) {
        info.err = "parseNetworkRouteSharePacket: packet data is empty";
        return info;
    }

    char* data = p->data.data();
    char addrSize = data[0];
    int offset = 1;
    info.address = p->data.mid(offset, addrSize);
    offset += addrSize;

    char srvSize = data[offset];
    offset += 1;
    info.service = p->data.mid(offset, srvSize);
    offset += srvSize;

    info.load = readINT16U((INT08U*)data);
    return info;
}

Packet* Router::composeNetQuery() {
    Packet* p = new Packet();
    p->net = Packet::NetState::QUERY;
    return p;
}

void Router::removeAddress(QString address) {
    QMutexLocker lock(&mMutex);
    remoteNodeMap.remove(address);
    foreach(QString service, serviceLoadMap.keys())
        serviceLoadMap[service].remove(address);
}

void Router::handleNetState(Channel* channel, Packet* packet) {
    switch (packet->net) {
    case Packet::NetState::ROUTE: {
        qDebug() << QString("router '%1' recv'd ROUTE update").arg(mAddress);
        // neighbor is sharing it's routing table
        RemoteNodeInfo info = parseNetRouteShare(packet);
        if (info.err.length() > 0) {
            qDebug() << info.err;
            return;
        }

        QString msg("NET_ROUTE [%1] to:%2, via:%3, cost:%4");
        qDebug() << msg.arg(mAddress, info.address, packet->srcAddress).arg(info.cost);
        if (info.cost == 0) { // zero cost routes are removed
            if (info.address == mAddress) {
                // TODO short delay
                shareNetState();
                return;
            } else if (remoteNodeMap.contains(info.address)) {
                // remove the route

                RemoteNodeInfo* localInfo = remoteNodeMap[info.address];
                removeAddress(info.address);
                for (Channel* ch : channels) {
                    if (ch != localInfo->channel) {
                        ch->send(packet);
                    }
                }
            }
        } else { // add or update a route
            QMutexLocker lock(&mMutex);
            RemoteNodeInfo* localInfo;
            if (remoteNodeMap.contains(info.address)) {
                localInfo = remoteNodeMap[info.address];
            } else {
                localInfo = new RemoteNodeInfo();
                localInfo->address = info.address;
                remoteNodeMap.insert(info.address, localInfo);
            }

//            localInfo.lastSeen = new Date();
            if (info.cost < localInfo->cost || localInfo->cost == 0) {
                localInfo->channel = channel;
                localInfo->cost = info.cost;
                localInfo->nextHop = info.nextHop;
                Packet* p = composeNetRouteShare(info.address, ++info.cost);
                for (Channel* ch : channels) {
                    if (ch != channel) {
                        ch->send(p);
                    }
                }
            }
        }
    } break;

    case Packet::NetState::SERVICE: {
        qDebug() << QString("router '%1' recv'd SERVICE update").arg(mAddress);
        ServiceNodeInfo serviceInfo = parseNetServiceShare(packet);
        if (serviceInfo.err.length() > 0) {
            qDebug() << "error parsing net service: " << serviceInfo.err;
            return;
        }
        {
            QMutexLocker lock(&mMutex);
            NodeLoad* nodeLoad = new NodeLoad();
            nodeLoad->load = serviceInfo.load;
            nodeLoad->lastSeen = QDate::currentDate();
            QMap<QString, NodeLoad*> loadMap;
            if (!serviceLoadMap.contains(serviceInfo.service)) {
                serviceLoadMap.insert(serviceInfo.service, loadMap);
            } else {
                loadMap = serviceLoadMap[serviceInfo.service];
                if (loadMap.contains(serviceInfo.address) &&
                        loadMap[serviceInfo.address]->load == nodeLoad->load) {
                    return; // drop redunant packet to avoid propagation loops
                }
            }

            loadMap.insert(serviceInfo.address, nodeLoad);

            if (serviceQueue.contains(serviceInfo.service)) {
                QList<Packet*> sq = serviceQueue[serviceInfo.service];
                qDebug() << QString("sending %1 queued packet(s) to '%2'").arg(sq.size()).arg(serviceInfo.address);
                if (remoteNodeMap.contains(serviceInfo.address)) {
                    RemoteNodeInfo* routeInfo = remoteNodeMap[serviceInfo.address];
                    for (Packet* p : sq) {
                        p->destAddress = serviceInfo.address;
                        p->nxtAddress = routeInfo->nextHop;
                        routeInfo->channel->send(p);
                    }
                    serviceQueue.remove(serviceInfo.service);
                } else {
                    qDebug() << QString("no route to discovered service %1").arg(serviceInfo.service);
                }
            }
        }
        // forward the service load
        for (Channel* ch : channels) {
            if (ch != channel) {
                ch->send(packet);
            }
        }
    } break;

    case Packet::NetState::QUERY:
        qDebug() << QString("router '%1' recv'd QUERY").arg(mAddress);
        for (Packet* p : exportRouteTable())
            channel->send(p);
        for (Packet* p : exportServiceTable())
            channel->send(p);
        break;
    }
}

void Router::onPacket(Channel* channel, Packet* packet) {
    if (packet->net != 0) {
        handleNetState(channel, packet);
    } else {
        send(packet);
    }
}

void Router::onChannelClose(Channel* channel) {
    removeChannel(channel);
}

void Router::addChannel(Channel* channel) {
    {
        QMutexLocker lock(&mMutex);
        channels.append(channel);
    }

    connect(channel, SIGNAL(onPacket(Channel*,Packet*)), this, SLOT(onPacket(Channel*,Packet*)), Qt::QueuedConnection);
    connect(channel, SIGNAL(onClose(Channel*)), this, SLOT(onChannelClose(Channel*)));

    qDebug() << QString("router '%1' sending QUERY").arg(mAddress);

    channel->send(composeNetQuery()); // immediately query the new connection
}

void Router::removeChannel(Channel* ch) {
    qDebug() << "router:RemoveChannel";
    {
        QMutexLocker lock(&mMutex);
        channels.remove(channels.indexOf(ch));
        // bcast the loss of routes through the channel
        foreach (QString address, remoteNodeMap.keys()) {
            qDebug() << QString("router:RemoveChannel address '%1'").arg(address);
            RemoteNodeInfo* nodeInfo = remoteNodeMap[address];
            if (nodeInfo->channel == ch) {
                removeAddress(address);
                foreach (Channel* c, channels) {
                    c->send(composeNetRouteShare(address, (short) 0));
                }
            }
        }
    }
}

void Router::registerService(QString service, PacketHandler* handler) {
    QMutexLocker lock(&mMutex);
    serviceHandlerMap.insert(service, handler);
}

void Router::unregisterService(QString service) {
    QMutexLocker lock(&mMutex);
    serviceHandlerMap.remove(service);
}

QList<Packet*> Router::exportRouteTable() {
    QList<Packet*> routes;
    routes.append(composeNetRouteShare(mAddress, (short) 1));
    foreach (QString address, remoteNodeMap.keys()) {
        RemoteNodeInfo* routeInfo = remoteNodeMap[address];
        routes.append(composeNetRouteShare(address, (short) (routeInfo->cost + 1)));
    }
    return routes;
}

QList<Packet*> Router::exportServiceTable() {
    QList<Packet*> services;
    foreach (QString service, serviceHandlerMap.keys()) {
        services.append(composeNetServiceShare(mAddress, service, (short) 0));
    }
    foreach (QString service, serviceLoadMap.keys()) {
        QMap<QString, NodeLoad*> loadMap = serviceLoadMap[service];
        foreach (QString address, loadMap.keys()) {
            NodeLoad* loadInfo = loadMap[address];
            services.append(composeNetServiceShare(address, service, loadInfo->load));
        }
    }
    return services;
}

void Router::shareNetState() {
    QList<Packet*> routes, services;
    {
        QMutexLocker lock(&mMutex);
        routes = exportRouteTable();
        services = exportServiceTable();
    }
    for (Channel* ch : channels) {
        for (Packet* p : routes)
            ch->send(p);
        for (Packet* p : services)
            ch->send(p);
    }
}
