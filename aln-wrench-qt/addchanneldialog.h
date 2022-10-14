#ifndef ADDCHANNELDIALOG_H
#define ADDCHANNELDIALOG_H

#include <QDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>

class AddChannelDialog : public QDialog
{
    Q_OBJECT
    QPlainTextEdit *textEdit;
    QLabel* errorLabel;
    QVBoxLayout* mainLayout;
    QPushButton *okButton;
public:
    AddChannelDialog(QWidget *parent = 0);

public slots:
    void okClicked();

signals:
    void connectRequest(QString);
};

#endif // ADDCHANNELDIALOG_H
