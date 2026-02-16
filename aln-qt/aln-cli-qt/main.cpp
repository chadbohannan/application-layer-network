#include <QCoreApplication>
#include <QTest>
#include "test_router.h"
#include "test_ping_service.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;

    // Run router tests
    {
        TestRouter testRouter;
        status |= QTest::qExec(&testRouter, argc, argv);
    }

    // Run ping service tests
    {
        TestPingService testPingService;
        status |= QTest::qExec(&testPingService, argc, argv);
    }

    return status;
}
