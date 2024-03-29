#ifndef CHANNEL_H
#define CHANNEL_H

#include <QObject>
#include "packet.h"

class Channel : public QObject {
    Q_OBJECT;
public:
    Channel(QObject* parent = 0);
    virtual bool send(Packet*) = 0;
    virtual bool listen() = 0;
    virtual void disconnect() = 0;

signals:
    void closing(Channel*);
    void packetReceived(Channel*, Packet*);
};

#endif // CHANNEL_H
