#include <QCoreApplication>
#include <QDebug>
#include <aln/router.h>
#include <aln/tcpchannel.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "ALN CLI Tool";
    qDebug() << "Starting router...";

    // Create a router with a unique address
    Router* router = new Router("cli-node-" + QUuid::createUuid().toString());
    qDebug() << "Router address:" << router->address();

    // TODO: Add CLI functionality here
    // - Command line argument parsing
    // - TCP listener setup
    // - Service registration
    // - Interactive command processing

    return app.exec();
}
