#include "advertiserthread.h"

#include <QUdpSocket>
#include <QUrl>

AdvertiserThread::AdvertiserThread(QString url, QString addr, int port, QObject* parent)
    : QThread(parent), mUrl(url), mAddr(addr), mPort(port) {}

void AdvertiserThread::setUrl(QString url) {
    mUrl = url;
}

void AdvertiserThread::stop() {
    mRun = false;
}

void AdvertiserThread::run() {
    if (mRun)
        return;
    mRun = true;
    qDebug() << "starting AdvertiserThread with:" << mUrl;
    QByteArray dgram = mUrl.toUtf8();
    QUdpSocket *udp = new QUdpSocket();
    while (mRun) { // TODO use QWaitCondition
        qDebug() << "Advertiser broadcasting '" << dgram <<"'";
        udp->writeDatagram(dgram.data(), dgram.size(), QHostAddress(mAddr), mPort);
        udp->flush();
        sleep(2);
    }
    qDebug() << "stopping AdvertiserThread with:" << mUrl;
    udp->close();
    udp->deleteLater();
}
