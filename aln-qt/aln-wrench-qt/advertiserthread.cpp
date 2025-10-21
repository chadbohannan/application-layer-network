#include "advertiserthread.h"

#include <QUdpSocket>
#include <QUrl>

AdvertiserThread::AdvertiserThread(QString url, QString addr, int port, int periodMs, QObject* parent)
    : QThread(parent), mUrl(url), mAddr(addr), mPort(port), mPeriodMs(periodMs) {}

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
    qDebug() << "starting AdvertiserThread with:" << mUrl << "every" << mPeriodMs << "ms";
    QByteArray dgram = mUrl.toUtf8();
    QUdpSocket *udp = new QUdpSocket();
    while (mRun) {
        qDebug() << "Advertiser broadcasting '" << dgram <<"'";
        udp->writeDatagram(dgram.data(), dgram.size(), QHostAddress(mAddr), mPort);
        udp->flush();

        // Sleep in small increments to allow faster stopping
        int elapsed = 0;
        while (mRun && elapsed < mPeriodMs) {
            int sleepTime = qMin(100, mPeriodMs - elapsed);
            msleep(sleepTime);
            elapsed += sleepTime;
        }
    }
    qDebug() << "stopping AdvertiserThread with:" << mUrl;
    udp->close();
    udp->deleteLater();
}
