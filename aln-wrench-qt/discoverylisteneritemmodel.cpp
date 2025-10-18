#include "discoverylisteneritemmodel.h"

DiscoveryListenerItem::DiscoveryListenerItem(QString address, int port, QUdpSocket* socket)
    : mAddress(address), mPort(port), mSocket(socket)
{
}

DiscoveryListenerItem::~DiscoveryListenerItem()
{
    if (mSocket) {
        if (mSocket->isOpen()) {
            mSocket->close();
        }
        delete mSocket;
    }
}

DiscoveryListenerItemModel::DiscoveryListenerItemModel(QList<DiscoveryListenerItem*> items, QObject *parent)
    : QAbstractItemModel{parent}, mItems(items)
{
}

QModelIndex DiscoveryListenerItemModel::index(int row, int column, const QModelIndex&) const {
    return createIndex(row, column);
}

QModelIndex DiscoveryListenerItemModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int DiscoveryListenerItemModel::rowCount(const QModelIndex &parent) const
{
    return mItems.length();
}

int DiscoveryListenerItemModel::columnCount(const QModelIndex &parent) const
{
    return NumDiscoveryListenerColumns;
}

Qt::ItemFlags DiscoveryListenerItemModel::flags(const QModelIndex &index) const {
    // Display-only mode
    return QAbstractItemModel::flags(index);
}

QVariant DiscoveryListenerItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch(section){
        case DiscoveryListenerColumn::ListenerAddress:
            return "Address";
        case DiscoveryListenerColumn::ListenerPort:
            return "Port";
        }
    return QVariant();
}

QVariant DiscoveryListenerItemModel::data(const QModelIndex &index, int role) const
{
    DiscoveryListenerItem* item = mItems.at(index.row());

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch(index.column()) {
    case DiscoveryListenerColumn::ListenerAddress:
        return item->address();
    case DiscoveryListenerColumn::ListenerPort:
        return item->port();
    }
    return QVariant();
}
