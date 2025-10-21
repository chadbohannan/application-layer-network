#ifndef LOCALCHANNEL_H
#define LOCALCHANNEL_H

#include "channel.h"

// LocalChannel is a utility to connect routers in multihop tests
class LocalChannel : public Channel
{
    Q_OBJECT
    LocalChannel* mBuddy;
public:
    LocalChannel(QObject* parent = 0);
    LocalChannel* buddy();

protected:
    LocalChannel(QObject* parent, LocalChannel* buddy);

public:
    bool send(Packet *);
    bool listen();
    void disconnect();

public slots:
    void doSend(Packet*);

signals:
    void queueSend(Packet*);

protected:
    void tx(Packet*);
};

#endif // LOCALCHANNEL_H
