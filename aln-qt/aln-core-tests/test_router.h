#ifndef TEST_ROUTER_H
#define TEST_ROUTER_H

#include <QObject>
#include <QTest>
#include <aln/router.h>
#include <aln/localchannel.h>
#include "lambda_packet_handler.h"

class TestRouter : public QObject
{
    Q_OBJECT

private slots:
    // Test setup/cleanup
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic router tests
    void testRouterCreation();
    void testAddChannel();
    void testLocalPacketDelivery();
    void testRemotePacketRouting();
    void testServiceRegistration();
    void testServiceInvocation();
    void testContextIDRouting();

private:
    Router* router1;
    Router* router2;
    LocalChannel* channel1;
    LocalChannel* channel2;
    QList<LambdaPacketHandler*> handlers; // Track handlers for cleanup
};

#endif // TEST_ROUTER_H
