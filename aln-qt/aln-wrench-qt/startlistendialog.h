#ifndef STARTLISTENDIALOG_H
#define STARTLISTENDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

class StartListenDialog : public QDialog
{
    Q_OBJECT

    QLineEdit* broadcastAddressLineEdit;
    QSpinBox* portSpinBox;
    QLabel* errorLabel;

public:
    StartListenDialog(QString defaultBroadcastAddr, int defaultPort, QWidget *parent = nullptr);

    QString broadcastAddress() const;
    int port() const;

private slots:
    void onOkClicked();
    void onCancelClicked();
};

#endif // STARTLISTENDIALOG_H
