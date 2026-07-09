#ifndef DISCOVERYLISTENERITEMMODEL_H
#define DISCOVERYLISTENERITEMMODEL_H

#include <QAbstractItemModel>
#include <QUdpSocket>

enum DiscoveryListenerColumn {
    ListenerAddress = 0,
    ListenerPort,
    NumDiscoveryListenerColumns
};

class DiscoveryListenerItem {
    QString mAddress;
    int mPort;
    QUdpSocket* mSocket;

public:
    DiscoveryListenerItem(QString address, int port, QUdpSocket* socket);
    ~DiscoveryListenerItem();

    QString address() const { return mAddress; }
    int port() const { return mPort; }
    QUdpSocket* socket() const { return mSocket; }
};

class DiscoveryListenerItemModel : public QAbstractItemModel
{
    Q_OBJECT

    QList<DiscoveryListenerItem*> mItems;

public:
    DiscoveryListenerItemModel(QList<DiscoveryListenerItem*> items, QObject *parent);

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // DISCOVERYLISTENERITEMMODEL_H
