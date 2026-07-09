#ifndef OPENPORTDIALOG_H
#define OPENPORTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

class OpenPortDialog : public QDialog
{
    Q_OBJECT

    QComboBox* interfaceComboBox;
    QSpinBox* portSpinBox;
    QLabel* errorLabel;

    QList<QString> interfaceAddresses;

public:
    OpenPortDialog(QList<QString> interfaces, QWidget *parent = nullptr);

    QString selectedInterface() const;
    int selectedPort() const;

private slots:
    void onOkClicked();
    void onCancelClicked();
};

#endif // OPENPORTDIALOG_H
