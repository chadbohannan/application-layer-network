#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <aln/packet.h>
#include <aln/channel.h>
#include <aln/tcpchannel.h>

#include <aln/router.h>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Router* alnRouter;
    QList<Channel*> channels;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    bool testFlag = false;

public slots:
    void onPacket(Packet*);
    void onAddChannelButtonClicked();
    void onConnectRequest(QString url);
};
#endif // MAINWINDOW_H
