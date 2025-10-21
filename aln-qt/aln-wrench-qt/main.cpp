#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

MainWindow* w = 0;
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (w) w->onDebugMessage(type, context, msg);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "aln-qt_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    w = new MainWindow();
    w->init();
    w->setWindowTitle("ALN Wrench Qt");
    w->show();
    return a.exec();
}
