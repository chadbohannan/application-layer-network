#ifndef LOGSERVICEITEMMODEL_H
#define LOGSERVICEITEMMODEL_H

#include <QAbstractItemModel>
#include <QDateTime>
#include <QList>

enum LogServiceColumn {
    Timestamp = 0,
    SourceAddress,
    DataPayload,
    NumLogServiceColumns
};

class LogServiceItem {
    QDateTime mTimestamp;
    QString mSourceAddress;
    QString mData;

public:
    LogServiceItem(QDateTime timestamp, QString sourceAddress, QString data)
        : mTimestamp(timestamp), mSourceAddress(sourceAddress), mData(data) {}

    QDateTime timestamp() const { return mTimestamp; }
    QString sourceAddress() const { return mSourceAddress; }
    QString data() const { return mData; }
};

class LogServiceItemModel : public QAbstractItemModel
{
    Q_OBJECT

    QList<LogServiceItem*> mItems;

public:
    explicit LogServiceItemModel(QList<LogServiceItem*> items, QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

#endif // LOGSERVICEITEMMODEL_H
