#ifndef TEST_PING_SERVICE_H
#define TEST_PING_SERVICE_H

#include <QObject>
#include <QTest>
#include <aln/router.h>
#include <aln/localchannel.h>
#include "lambda_packet_handler.h"

/**
 * Test suite that mimics the Go example at go/examples/local_host/main.go
 * Tests the ping service implementation
 */
class TestPingService : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Ping service tests (mimics Go example behavior)
    void testPingServiceRegistration();
    void testPingServiceResponse();
    void testPingServiceContextID();
    void testMultiplePingRequests();
    void testTwoRoutersPingPong();

private:
    Router* serverRouter;
    Router* clientRouter;
    LocalChannel* channel1;
    LocalChannel* channel2;
    QList<LambdaPacketHandler*> handlers; // Track handlers for cleanup

    void setupPingService(Router* router);
};

#endif // TEST_PING_SERVICE_H
