#include "logserviceitemmodel.h"

LogServiceItemModel::LogServiceItemModel(QList<LogServiceItem*> items, QObject *parent)
    : QAbstractItemModel{parent}, mItems(items)
{
}

QModelIndex LogServiceItemModel::index(int row, int column, const QModelIndex&) const {
    return createIndex(row, column);
}

QModelIndex LogServiceItemModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int LogServiceItemModel::rowCount(const QModelIndex &parent) const
{
    return mItems.length();
}

int LogServiceItemModel::columnCount(const QModelIndex &parent) const
{
    return NumLogServiceColumns;
}

Qt::ItemFlags LogServiceItemModel::flags(const QModelIndex &index) const {
    // Display-only mode (not editable)
    return QAbstractItemModel::flags(index);
}

QVariant LogServiceItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch(section){
        case LogServiceColumn::Timestamp:
            return "Timestamp";
        case LogServiceColumn::SourceAddress:
            return "Source Address";
        case LogServiceColumn::DataPayload:
            return "Data";
        }
    return QVariant();
}

QVariant LogServiceItemModel::data(const QModelIndex &index, int role) const
{
    LogServiceItem* item = mItems.at(index.row());

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch(index.column()) {
    case LogServiceColumn::Timestamp:
        return item->timestamp().toString(Qt::ISODateWithMs);
    case LogServiceColumn::SourceAddress:
        return item->sourceAddress();
    case LogServiceColumn::DataPayload:
        return item->data();
    }
    return QVariant();
}
