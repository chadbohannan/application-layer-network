#include "advertiserthread.h"

#include <QUdpSocket>
#include <QUrl>

AdvertiserThread::AdvertiserThread(QString url, QString addr, int port, QObject* parent)
    : QThread(parent), mUrl(url), mAddr(addr), mPort(port) {}

void AdvertiserThread::stop() {
    mRun = false;
}

void AdvertiserThread::run() {
    if (mRun)
        return;
    mRun = true;
    qDebug() << "AdvertiserThread starting for:" << mUrl;
    QByteArray dgram = mUrl.toUtf8();
    QUdpSocket *udp = new QUdpSocket();
    while (mRun) { // TODO use QWaitCondition
        udp->writeDatagram(dgram.data(), dgram.size(), QHostAddress(mAddr), mPort);
        udp->flush();
        sleep(2);
    }
    udp->close();
    udp->deleteLater();
}
