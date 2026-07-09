#ifndef STARTADVERTISEMENTDIALOG_H
#define STARTADVERTISEMENTDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class StartAdvertisementDialog : public QDialog
{
    Q_OBJECT

    QSpinBox* periodSpinBox;
    QLineEdit* broadcastAddressLineEdit;
    QSpinBox* broadcastPortSpinBox;
    QTextEdit* messageTextEdit;
    QComboBox* connectionStringComboBox;
    QLabel* errorLabel;

    QStringList availableConnectionStrings;

public:
    StartAdvertisementDialog(QStringList connectionStrings, QString defaultBroadcastAddr, int defaultBroadcastPort, QWidget *parent = nullptr);

    int broadcastPeriodMs() const;
    QString broadcastAddress() const;
    int broadcastPort() const;
    QString message() const;

private slots:
    void onConnectionStringSelected(int index);
    void onOkClicked();
    void onCancelClicked();
};

#endif // STARTADVERTISEMENTDIALOG_H
