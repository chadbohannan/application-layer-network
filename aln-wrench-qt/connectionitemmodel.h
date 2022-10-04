#ifndef CONNECTIONITEMMODEL_H
#define CONNECTIONITEMMODEL_H

#include <aln/channel.h>
#include <QAbstractItemModel>

enum ConnectionItemColumn {
    Info = 0,
    NumConnectionItemColumns
};

class ConnectionItem {
    QString mInfo;
    Channel* mChannel;
public:
    ConnectionItem(Channel* ch, QString info);
    QString info() { return mInfo; }
    Channel* channel() { return mChannel; }
};

class ConnectionItemModel : public QAbstractItemModel
{
    Q_OBJECT

    QList<ConnectionItem*> mItems;

public:
//    ConnectionItemModel(QObject *parent = nullptr);
    ConnectionItemModel(QList<ConnectionItem*> items, QObject *parent);

public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

#endif // CONNECTIONITEMMODEL_H
