#include "connectionitemmodel.h"

ConnectionItem::ConnectionItem(Channel *ch, QString info)
    : mInfo(info), mChannel(ch) {}


ConnectionItemModel::ConnectionItemModel(QList<ConnectionItem*> items, QObject *parent)
    : QAbstractItemModel{parent}, mItems(items)
{

}

QModelIndex ConnectionItemModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column);
}

QModelIndex ConnectionItemModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

int ConnectionItemModel::rowCount(const QModelIndex &parent) const
{
    return mItems.length();
}

int ConnectionItemModel::columnCount(const QModelIndex &parent) const
{
    return ConnectionItemColumn::NumConnectionItemColumns;
}

Qt::ItemFlags ConnectionItemModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}

QVariant ConnectionItemModel::data(const QModelIndex &index, int role) const
{
    ConnectionItem* item = mItems.at(index.row());
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();
    switch(index.column()) {
    case ConnectionItemColumn::Info:
        return item->info();
    }
}

bool ConnectionItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

QVariant ConnectionItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch(section){
        case ConnectionItemColumn::Info:
            return "info";

        }
    return QVariant();
}


