#ifndef KNOWNCONNECTIONITEMMODEL_H
#define KNOWNCONNECTIONITEMMODEL_H

#include <QAbstractItemModel>
#include <aln/channel.h>

enum KnownConnectionColumn {
    ConnectionUrl = 0,
    ConnectionStatusColumn,
    NumKnownConnectionColumns
};

enum ConnectionStatus {
    Disconnected = 0,
    Connected
};

class KnownConnectionItem {
    QString mUrl;
    ConnectionStatus mStatus;
    Channel* mChannel;

public:
    KnownConnectionItem(QString url, ConnectionStatus status = Disconnected, Channel* channel = nullptr);

    QString url() const { return mUrl; }
    ConnectionStatus status() const { return mStatus; }
    Channel* channel() const { return mChannel; }

    void setStatus(ConnectionStatus status) { mStatus = status; }
    void setChannel(Channel* channel) { mChannel = channel; }
};

class KnownConnectionItemModel : public QAbstractItemModel
{
    Q_OBJECT

    QList<KnownConnectionItem*> mItems;

public:
    KnownConnectionItemModel(QList<KnownConnectionItem*> items, QObject *parent);

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // KNOWNCONNECTIONITEMMODEL_H
