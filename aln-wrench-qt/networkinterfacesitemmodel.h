#ifndef NETWORKINTERFACESITEMMODEL_H
#define NETWORKINTERFACESITEMMODEL_H

#include <QAbstractItemModel>
#include <QObject>

enum Column {
    Type = 0,
    Address,
    ListenPort,
    Listen,
    BroadcastPort,
    Advertise,
    BroadcastListen,
    NumColumns
};

class NetworkInterfaceItem {
    QString mType;
    QString mListenAddress;
    QString mBroadcastAddress;
    int mListenPort;
    int mBroadcastPort;
    bool mIsListening;
    bool mIsAdvertising;
    bool mIsBroadcastListening;

public:
    NetworkInterfaceItem(QString typ, QString addr, int port, QString bcastAddr = QString())
        : mType(typ), mListenAddress(addr), mBroadcastAddress(bcastAddr), mListenPort(port),
          mIsListening(false), mIsAdvertising(false), mIsBroadcastListening(false) {
        mBroadcastPort = port + 1;
    }
    QString typ() const { return mType; }
    QString listenAddress() const { return mListenAddress; }
    int listenPort() const { return mListenPort; }
    void setListenPort(int value) { mListenPort = value;}
    bool isListening() { return mIsListening; }
    void setIsListening(bool value) { mIsListening = value; }

    QString broadcastAddress() { return mBroadcastAddress; }
    int broadcastPort() const { return mBroadcastPort; }
    void setAdvertisingPort(int value) { mBroadcastPort = value;}
    bool isAdvertising() { return mIsAdvertising; }
    void setIsAdvertising(bool value) { mIsAdvertising = value; }
    void setIsBroadcastListening(bool value) { mIsBroadcastListening = value; }
    bool isBroadcastListening() { return mIsBroadcastListening; }
};

class NetworkInterfacesItemModel : public QAbstractItemModel
{
    Q_OBJECT
    QList<NetworkInterfaceItem*> interfaces;
public:
    explicit NetworkInterfacesItemModel(QList<NetworkInterfaceItem*> data, QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

signals:
    void listenRequest(QString interface, int port, bool enable);
    void advertiseRequest(QString url, QString interface, int port, bool enable);
    void advertiseListenRequest(QString interface, int port, bool enable);
};

#endif // NETWORKINTERFACESITEMMODEL_H
