#include "knownconnectionitemmodel.h"

KnownConnectionItem::KnownConnectionItem(QString url, ConnectionStatus status, Channel* channel)
    : mUrl(url), mStatus(status), mChannel(channel)
{
}

KnownConnectionItemModel::KnownConnectionItemModel(QList<KnownConnectionItem*> items, QObject *parent)
    : QAbstractItemModel{parent}, mItems(items)
{
}

QModelIndex KnownConnectionItemModel::index(int row, int column, const QModelIndex&) const {
    return createIndex(row, column);
}

QModelIndex KnownConnectionItemModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int KnownConnectionItemModel::rowCount(const QModelIndex &parent) const
{
    return mItems.length();
}

int KnownConnectionItemModel::columnCount(const QModelIndex &parent) const
{
    return NumKnownConnectionColumns;
}

Qt::ItemFlags KnownConnectionItemModel::flags(const QModelIndex &index) const {
    // Display-only mode
    return QAbstractItemModel::flags(index);
}

QVariant KnownConnectionItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch(section){
        case KnownConnectionColumn::ConnectionUrl:
            return "URL";
        case KnownConnectionColumn::ConnectionStatusColumn:
            return "Status";
        }
    return QVariant();
}

QVariant KnownConnectionItemModel::data(const QModelIndex &index, int role) const
{
    KnownConnectionItem* item = mItems.at(index.row());

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch(index.column()) {
    case KnownConnectionColumn::ConnectionUrl:
        return item->url();
    case KnownConnectionColumn::ConnectionStatusColumn:
        return item->status() == Connected ? "Connected" : "Disconnected";
    }
    return QVariant();
}
