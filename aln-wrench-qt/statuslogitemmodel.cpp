#include "statuslogitemmodel.h"

StatusLogItemModel::StatusLogItemModel(QList<StatusLogItem*> items, QObject *parent)
    : QAbstractItemModel{parent}, mItems(items)
{
}

QModelIndex StatusLogItemModel::index(int row, int column, const QModelIndex&) const {
    return createIndex(row, column);
}

QModelIndex StatusLogItemModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int StatusLogItemModel::rowCount(const QModelIndex &parent) const
{
    return mItems.length();
}

int StatusLogItemModel::columnCount(const QModelIndex &parent) const
{
    return NumStatusLogColumns;
}

Qt::ItemFlags StatusLogItemModel::flags(const QModelIndex &index) const {
    // Display-only mode (not editable)
    return QAbstractItemModel::flags(index);
}

QVariant StatusLogItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch(section){
        case StatusLogColumn::StatusTimestamp:
            return "Timestamp";
        case StatusLogColumn::StatusLevel:
            return "Level";
        case StatusLogColumn::StatusData:
            return "Data";
        }
    return QVariant();
}

QVariant StatusLogItemModel::data(const QModelIndex &index, int role) const
{
    StatusLogItem* item = mItems.at(index.row());

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch(index.column()) {
    case StatusLogColumn::StatusTimestamp:
        return item->timestamp().toString(Qt::ISODateWithMs);
    case StatusLogColumn::StatusLevel:
        return item->level();
    case StatusLogColumn::StatusData:
        return item->data();
    }
    return QVariant();
}
