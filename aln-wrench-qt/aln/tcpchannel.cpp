#include "tcpchannel.h"
#include "ax25frame.h"
#include <QDebug>

TcpChannel::TcpChannel(QTcpSocket* s)
    : socket(s)
{
    if (socket->isOpen()) {
//        onConnected();
        QMetaObject::invokeMethod(this, "onConnected", Qt::QueuedConnection);
    } else {
        connect(socket, SIGNAL(connected()), this, SLOT(onConnected()));
    }
    connect(socket, SIGNAL(readyRead()), this, SLOT(onSocketDataReady()));
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(onSocketError(int)));

    parser = new Parser;
    connect(parser, SIGNAL(onPacket()), this, SLOT(onPacketParsed()));
}

void TcpChannel::onPacketParsed(Packet* p) {
    emit onPacket(this, p);
}

QString TcpChannel::lastError() {
    return err;
}

bool TcpChannel::send(Packet* p) {
    if (!socket->isOpen()) {
        packetQueue.append(p);
        err = "Socket not open; packet queued";
        return false;
    }

    try {
        socket->write(toAx25Buffer(p->toByteArray()));
    } catch (...) {
        err = QString("socket write err:%1").arg(socket->errorString());
        return false;
    }

    return true;
}

void TcpChannel::onConnected() {
    qDebug() << "TcpChannel::onConnected";
    foreach(Packet* p, packetQueue) {
        qDebug() << "sending queued packet";
        send(p);
    }
    packetQueue.clear();
}

bool TcpChannel::listen() {
    if (!socket-> isOpen()) {
        err = "Socket read error: socket is not open";
        return false;
    }

    return true;
}

void TcpChannel::disconnect() {
    QObject::disconnect(socket, &QTcpSocket::readyRead, this, &TcpChannel::onSocketDataReady);
    emit onClose();
    if (socket->isOpen())
        socket->close();
}

void TcpChannel::onSocketError(int errCode) {
    qDebug() << "socket error code:" << errCode;
}

void TcpChannel::onSocketDataReady() {
    QByteArray data = socket->readAll();
    qDebug() << "recieved:" << data;
    parser->read(data);
}

void TcpChannel::onSocketClose() {
    disconnect();
}

