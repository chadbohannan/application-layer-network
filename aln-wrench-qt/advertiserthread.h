#ifndef ADVERTISERTHREAD_H
#define ADVERTISERTHREAD_H

#include <QThread>

class AdvertiserThread : public QThread
{
    Q_OBJECT
    bool mRun = false;
    QString mUrl;
    QString mAddr;
    int mPort;
    int mPeriodMs;
public:
    AdvertiserThread(QString url, QString addr, int port, int periodMs, QObject* parent = nullptr);
    void setUrl(QString);
    void stop();

    // QThread interface
protected:
    void run();
};

#endif // ADVERTISERTHREAD_H
