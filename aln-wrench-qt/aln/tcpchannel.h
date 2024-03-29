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
    QString peerName();

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
    void onSocketError(QAbstractSocket::SocketError);
};

#endif // TCPCHANNEL_H
