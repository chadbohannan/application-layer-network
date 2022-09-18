#ifndef TCPCHANNEL_H
#define TCPCHANNEL_H

#include "channel.h"
#include "parser.h"
#include <QTcpSocket>


class TcpChannel : public Channel {
    Q_OBJECT;
private:
    Parser* parser;
    QTcpSocket* socket;
    QString err;
    QList<Packet*> packetQueue;

public:
    TcpChannel(QTcpSocket*, QObject* = 0);
    QString lastError();

    // AlnChannel interface
public:
    bool send(Packet*);
    bool listen();
    void disconnect();

private slots:
    void onSocketDataReady();
    void onSocketClose();
    void onPacketParsed(Packet*);
    void onConnected();
    void onSocketError(int);
};

#endif // TCPCHANNEL_H
