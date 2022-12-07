#include "tcpchannel.h"
#include "ax25frame.h"
#include <QDebug>

TcpChannel::TcpChannel(QTcpSocket* s, QObject* parent)
    : Channel(parent), socket(s)
{
    if (socket->isOpen()) {
        QMetaObject::invokeMethod(this, "onConnected", Qt::QueuedConnection);
    } else {
        connect(socket, SIGNAL(connected()), this, SLOT(onConnected()));
    }
    connect(socket, SIGNAL(readyRead()), this, SLOT(onSocketDataReady()));
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(disconnected()), this, SLOT(onSocketClose()));

    parser = new Parser;
    connect(parser, SIGNAL(onPacket(Packet*)), this, SLOT(onPacketParsed(Packet*)));
}

void TcpChannel::onPacketParsed(Packet* p) {
    emit packetReceived(this, p);
}

QString TcpChannel::lastError() {
    return err;
}

QString TcpChannel::peerName() {
    return this->socket->peerAddress().toString();
}

bool TcpChannel::send(Packet* p) {
    if (!socket->isOpen()) {
        packetQueue.append(p);
        err = "Socket not open; packet queued";
        qDebug() << err;
        return false;
    }

    try {
        socket->write(toAx25Buffer(p->toByteArray()));
    } catch (...) {
        err = QString("socket write err:%1").arg(socket->errorString());
        qDebug() << "TCPChannel::send err:"<< err << ", " << peerName();
        return false;
    }
    delete p;
    return true;
}

void TcpChannel::onConnected() {
    qDebug() << "TcpChannel connected";
    foreach(Packet* p, packetQueue) {
        qDebug() << "TcpChannel sending queued packet";
        send(p);
    }
    packetQueue.clear();
}

bool TcpChannel::listen() {
    if (!socket-> isOpen()) {
        err = "Socket read error: socket is not open";
        qDebug() << "TcpChannel:" << err;
        return false;
    }
    return true;
}

void TcpChannel::disconnect() {
    QObject::disconnect(socket, &QTcpSocket::readyRead, this, &TcpChannel::onSocketDataReady);
    emit closing(this);
    if (socket->isOpen())
        socket->close();
}

void TcpChannel::onSocketError(QAbstractSocket::SocketError errCode) {
    qInfo() << "socket closed: " << errCode;
    disconnect();
}

void TcpChannel::onSocketDataReady() {
    parser->read(socket->readAll());
}

void TcpChannel::onSocketClose() {
    disconnect();
}
