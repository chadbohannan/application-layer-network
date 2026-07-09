#ifndef STATUSLOGITEMMODEL_H
#define STATUSLOGITEMMODEL_H

#include <QAbstractItemModel>
#include <QDateTime>
#include <QString>
#include <QList>

enum StatusLogColumn {
    StatusTimestamp = 0,
    StatusLevel,
    StatusData,
    NumStatusLogColumns
};

class StatusLogItem {
    QDateTime mTimestamp;
    QString mLevel;
    QString mData;

public:
    StatusLogItem(QDateTime timestamp, QString level, QString data)
        : mTimestamp(timestamp), mLevel(level), mData(data) {}

    QDateTime timestamp() const { return mTimestamp; }
    QString level() const { return mLevel; }
    QString data() const { return mData; }
};

class StatusLogItemModel : public QAbstractItemModel
{
    Q_OBJECT
    QList<StatusLogItem*> mItems;

public:
    explicit StatusLogItemModel(QList<StatusLogItem*> items, QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

#endif // STATUSLOGITEMMODEL_H
