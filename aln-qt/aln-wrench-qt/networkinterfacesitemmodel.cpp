#include "networkinterfacesitemmodel.h"

#include <QColor>
#include <QSize>

NetworkInterfacesItemModel::NetworkInterfacesItemModel(QList<NetworkInterfaceItem*> data, QObject *parent)
    : QAbstractItemModel{parent}, interfaces(data) { }

QModelIndex NetworkInterfacesItemModel::index(int row, int column, const QModelIndex&) const {
    return createIndex(row, column);
}

QModelIndex NetworkInterfacesItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int NetworkInterfacesItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return interfaces.length();
}

int NetworkInterfacesItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return NumColumns;
}

Qt::ItemFlags NetworkInterfacesItemModel::flags(const QModelIndex &index) const {
    // Display-only mode - no editing allowed
    return QAbstractItemModel::flags(index);
}

QVariant NetworkInterfacesItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch(section){
        case Column::Type:
            return "type";
        case Column::Scheme:
            return "scheme";
        case Column::Address:
            return "interface";
        case Column::ListenPort:
            return "port";
        }
    return QVariant();
}

QVariant NetworkInterfacesItemModel::data(const QModelIndex &index, int role) const
{
    NetworkInterfaceItem* nim =  interfaces.at(index.row());

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();
    switch(index.column()) {
    case Column::Type:
        return nim->typ();
    case Column::Scheme:
        return "tcp+aln";
    case Column::Address:
        return nim->listenAddress();
    case Column::ListenPort:
        return nim->listenPort();
    }
    return QVariant();
}

bool NetworkInterfacesItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    // No editing allowed - all changes handled via OpenPortDialog
    return false;
}
