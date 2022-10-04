#include "localchannel.h"

LocalChannel::LocalChannel(QObject* parent) : Channel(parent) {
    mBuddy = new LocalChannel(parent, this);
    connect(this, SIGNAL(queueSend(Packet*)), this, SLOT(doSend(Packet*)), Qt::QueuedConnection);
}

LocalChannel::LocalChannel(QObject* parent, LocalChannel* buddy) : Channel(parent) {
    mBuddy = buddy;
    connect(this, SIGNAL(queueSend(Packet*)), this, SLOT(doSend(Packet*)), Qt::QueuedConnection);
}

LocalChannel* LocalChannel::buddy() {
    return mBuddy;
}

bool LocalChannel::send(Packet* p) {
    mBuddy->tx(p);
    return true;
}

void LocalChannel::doSend(Packet* p) {
    emit packetReceived(this, p);
}

void LocalChannel::tx(Packet* p) {
    emit queueSend(p);
}

bool LocalChannel::listen() {
    return true;
}

void LocalChannel::disconnect()
{
    emit closing(this);
}
