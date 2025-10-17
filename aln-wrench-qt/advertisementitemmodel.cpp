#include "advertisementitemmodel.h"

AdvertisementItem::AdvertisementItem(QString message, int periodMs, QString broadcastAddr, int broadcastPort, AdvertiserThread* thread)
    : mMessage(message), mPeriodMs(periodMs), mBroadcastAddress(broadcastAddr),
      mBroadcastPort(broadcastPort), mThread(thread)
{
    mStartTime = QDateTime::currentDateTime();
}

AdvertisementItem::~AdvertisementItem()
{
    if (mThread) {
        mThread->stop();
        // Wait up to 3 seconds for thread to finish
        // Thread wakes every 100ms to check stop flag, so this should be plenty
        if (!mThread->wait(3000)) {
            qWarning() << "Advertisement thread did not stop gracefully, terminating...";
            mThread->terminate();
            mThread->wait();
        }
        delete mThread;
    }
}

AdvertisementItemModel::AdvertisementItemModel(QList<AdvertisementItem*> items, QObject *parent)
    : QAbstractItemModel{parent}, mItems(items)
{
}

QModelIndex AdvertisementItemModel::index(int row, int column, const QModelIndex&) const {
    return createIndex(row, column);
}

QModelIndex AdvertisementItemModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int AdvertisementItemModel::rowCount(const QModelIndex &parent) const
{
    return mItems.length();
}

int AdvertisementItemModel::columnCount(const QModelIndex &parent) const
{
    return NumAdvertisementColumns;
}

Qt::ItemFlags AdvertisementItemModel::flags(const QModelIndex &index) const {
    // Display-only mode
    return QAbstractItemModel::flags(index);
}

QVariant AdvertisementItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch(section){
        case AdvertisementColumn::AdMessage:
            return "Message";
        case AdvertisementColumn::AdPeriod:
            return "Period (s)";
        case AdvertisementColumn::AdStarted:
            return "Started";
        }
    return QVariant();
}

QVariant AdvertisementItemModel::data(const QModelIndex &index, int role) const
{
    AdvertisementItem* item = mItems.at(index.row());

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch(index.column()) {
    case AdvertisementColumn::AdMessage:
        return item->message();
    case AdvertisementColumn::AdPeriod:
        return item->periodMs() / 1000;
    case AdvertisementColumn::AdStarted:
        return item->startTime().toString("hh:mm:ss");
    }
    return QVariant();
}
