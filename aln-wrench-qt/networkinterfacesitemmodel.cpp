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
    return QModelIndex();
}

int NetworkInterfacesItemModel::rowCount(const QModelIndex &parent) const
{
    return interfaces.length();
}

int NetworkInterfacesItemModel::columnCount(const QModelIndex &parent) const
{
    return NumColumns;
}

Qt::ItemFlags NetworkInterfacesItemModel::flags(const QModelIndex &index) const {
    switch(index.column()) {
    case Column::ListenPort: {
            NetworkInterfaceItem* item = interfaces.at(index.row());
            if (!item->isListening())
                return QAbstractItemModel::flags(index).setFlag(Qt::ItemIsEditable);
        } break;
    case Column::Listen:
        return QAbstractItemModel::flags(index).setFlag(Qt::ItemIsEditable);
//    case Column::BroadcastPort: {
//            NetworkInterfaceItem* item = interfaces.at(index.row());
//            if (!item->isAdvertising())
//                return QAbstractItemModel::flags(index).setFlag(Qt::ItemIsEditable);
//        } break;
//    case Column::Advertise: {
//            NetworkInterfaceItem* item = interfaces.at(index.row());
//            if (item->isListening())
//                return QAbstractItemModel::flags(index).setFlag(Qt::ItemIsEditable);
//        } break;
//    case Column::BroadcastListen:
//        return QAbstractItemModel::flags(index).setFlag(Qt::ItemIsEditable);
    }
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
        case Column::Listen:
            return "enable";
//        case Column::BroadcastPort:
//            return "bcast\nport";
//        case Column::Advertise:
//            return "bcast\nenable";
//        case Column::BroadcastListen:
//            return "bcast\nlisten";
        }
    return QVariant();
}

QVariant NetworkInterfacesItemModel::data(const QModelIndex &index, int role) const
{
    NetworkInterfaceItem* nim =  interfaces.at(index.row());
    switch (role) {
    case Qt::BackgroundRole:
        switch(index.column()) {
        case Column::ListenPort:
            if (nim->isListening())
                return QVariant(QColor(Qt::gray));
            break;
        case Column::Scheme:
            break;
//        case Column::BroadcastPort:
//            if (nim->isAdvertising())
//                return QVariant(QColor(Qt::gray));
//            break;
//        case Column::Advertise:
//            if (!nim->isListening())
//                return QVariant(QColor(Qt::gray));
//            break;
        }
    }

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
    case Column::Listen:
        return nim->isListening();
//    case Column::BroadcastPort:
//        return nim->broadcastPort();
//    case Column::Advertise:
//        return nim->isAdvertising();
//    case Column::BroadcastListen:
//        return nim->isBroadcastListening();
    }
}

bool NetworkInterfacesItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch(role) {
    case Qt::EditRole: break;
    default: return false;
    }

    NetworkInterfaceItem* item = interfaces.at(index.row());
    switch(index.column()){
    case Column::Scheme: {
            qDebug() << "TODO setData Column::Scheme";
        } return true;
    case Column::ListenPort: {
            bool ok = false;
            int port = value.toInt(&ok);
            if (!ok) return false;
            item->setListenPort(port);
            emit dataChanged(index, index);
        } return true;
    case Column::Listen: {
            bool enable = value.toBool();
            item->setIsListening(enable);
            emit listenRequest("tcp+aln", item->listenAddress(), item->listenPort(), enable);
            emit dataChanged(createIndex(index.row(), 0),
                             createIndex(index.row(), Column::NumColumns-1));
        } return true;
//    case Column::BroadcastPort: {
//            bool enable = value.toBool();
//            item->setAdvertisingPort(enable);
//            emit dataChanged(index, index);
//        } return true;
//    case Column::Advertise: {
//            bool enable = value.toBool();
//            item->setIsAdvertising(enable);
//            QString url = QString("tcp+aln://%1:%2").arg(item->listenAddress()).arg(item->listenPort());
//            emit advertiseRequest(url, item->broadcastAddress(), item->broadcastPort(), enable);
//            emit dataChanged(createIndex(index.row(), 0),
//                             createIndex(index.row(), Column::NumColumns-1));
//        } return true;
//    case Column::BroadcastListen: {
//            bool enable = value.toBool();
//            item->setIsBroadcastListening(enable);
//            emit advertiseListenRequest(item->listenAddress(), item->listenPort(), enable);
//            emit dataChanged(createIndex(index.row(), 0),
//                             createIndex(index.row(), Column::NumColumns-1));
//        } return true;
    }

    return false;
}
