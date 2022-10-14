#ifndef PACKETSENDDIALOG_H
#define PACKETSENDDIALOG_H

#include "aln/router.h"
#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>

class PacketSendDialog : public QDialog
{
    Q_OBJECT

    Router* router;
    short contextID;
    QString response;

    QLineEdit* destLineEdit;
    QLineEdit* serviceLineEdit;
    QLineEdit* dataLineEdit;
    QLineEdit* responseLineEdit;

public:
    PacketSendDialog(Router* alnRouter, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void setDest(QString destAddress);
    void setService(QString serviceName);
    void setData(QString data);
    void setResponse(QString resp);

public slots:
    void onSendClicked();
    void onCloseClicked();
    void onClose();

protected:
    QLayout* createLayout();
};

#endif // PACKETSENDDIALOG_H
