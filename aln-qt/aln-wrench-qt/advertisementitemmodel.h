#ifndef ADVERTISEMENTITEMMODEL_H
#define ADVERTISEMENTITEMMODEL_H

#include "advertiserthread.h"
#include <QAbstractItemModel>
#include <QDateTime>

enum AdvertisementColumn {
    AdMessage = 0,
    AdPeriod,
    AdStarted,
    NumAdvertisementColumns
};

class AdvertisementItem {
    QString mMessage;
    int mPeriodMs;
    QDateTime mStartTime;
    AdvertiserThread* mThread;
    QString mBroadcastAddress;
    int mBroadcastPort;

public:
    AdvertisementItem(QString message, int periodMs, QString broadcastAddr, int broadcastPort, AdvertiserThread* thread);
    ~AdvertisementItem();

    QString message() const { return mMessage; }
    int periodMs() const { return mPeriodMs; }
    QDateTime startTime() const { return mStartTime; }
    AdvertiserThread* thread() const { return mThread; }
    QString broadcastAddress() const { return mBroadcastAddress; }
    int broadcastPort() const { return mBroadcastPort; }
};

class AdvertisementItemModel : public QAbstractItemModel
{
    Q_OBJECT

    QList<AdvertisementItem*> mItems;

public:
    AdvertisementItemModel(QList<AdvertisementItem*> items, QObject *parent);

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // ADVERTISEMENTITEMMODEL_H
