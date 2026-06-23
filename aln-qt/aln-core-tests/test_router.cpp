#include "test_router.h"
#include <QSignalSpy>
#include <QTimer>

void TestRouter::initTestCase()
{
    // Called once before all tests
}

void TestRouter::cleanupTestCase()
{
    // Called once after all tests
}

void TestRouter::init()
{
    // Called before each test - set up fresh routers
    router1 = new Router("router-1");
    router2 = new Router("router-2");
    channel1 = nullptr;
    channel2 = nullptr;
}

void TestRouter::cleanup()
{
    // Called after each test - clean up
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();

    if (channel1) delete channel1;
    if (channel2) delete channel2;
    if (router1) delete router1;
    if (router2) delete router2;
}

void TestRouter::testRouterCreation()
{
    // Test: Router should be created with an address
    QVERIFY(router1 != nullptr);
    QCOMPARE(router1->address(), QString("router-1"));
}

void TestRouter::testAddChannel()
{
    // Test: Should be able to add a local channel to router
    channel1 = new LocalChannel();
    channel2 = channel1->buddy();

    router1->addChannel(channel1);

    // Channel should be connected
    QVERIFY(channel1 != nullptr);
    QVERIFY(channel2 != nullptr);
}

void TestRouter::testLocalPacketDelivery()
{
    // Test: Router should deliver packets to local service handlers
    bool packetReceived = false;
    QByteArray receivedData;

    // Register a service handler using wrapper
    auto* handler = new LambdaPacketHandler([&](Packet* p) {
        packetReceived = true;
        receivedData = p->data;
    });
    handlers.append(handler);
    router1->registerService("test-service", handler);

    // Send a packet to the local service
    Packet* packet = new Packet("router-1", "test-service", QByteArray("hello"));
    router1->send(packet);

    // Process events to allow signal/slot delivery
    QTest::qWait(10);

    QVERIFY(packetReceived);
    QCOMPARE(receivedData, QByteArray("hello"));
}

void TestRouter::testRemotePacketRouting()
{
    // Test: Packet should be routed from router1 to router2 via local channel
    channel1 = new LocalChannel();
    channel2 = channel1->buddy();

    router1->addChannel(channel1);
    router2->addChannel(channel2);

    bool packetReceived = false;
    QString receivedData;

    // Register service on router2
    auto* handler = new LambdaPacketHandler([&](Packet* p) {
        packetReceived = true;
        receivedData = QString(p->data);
    });
    handlers.append(handler);
    router2->registerService("remote-service", handler);

    // Wait for route advertisement to propagate
    QTest::qWait(100);

    // Send packet from router1 to router2's service
    Packet* packet = new Packet("router-2", "remote-service", QByteArray("remote-hello"));
    packet->srcAddress = "router-1";
    router1->send(packet);

    // Wait for packet to be routed
    QTest::qWait(50);

    QVERIFY(packetReceived);
    QCOMPARE(receivedData, QString("remote-hello"));
}

void TestRouter::testServiceRegistration()
{
    // Test: Service should be registered and advertised
    int handlerCallCount = 0;

    auto* handler = new LambdaPacketHandler([&](Packet* p) {
        Q_UNUSED(p);
        handlerCallCount++;
    });
    handlers.append(handler);
    router1->registerService("my-service", handler);

    // Send a packet to the service
    Packet* packet = new Packet("router-1", "my-service", QByteArray("test"));
    router1->send(packet);

    QTest::qWait(10);

    QCOMPARE(handlerCallCount, 1);
}

void TestRouter::testServiceInvocation()
{
    // Test: Service handler should receive correct packet data
    QString receivedService;
    QByteArray receivedData;
    QString receivedSrc;

    auto* handler = new LambdaPacketHandler([&](Packet* p) {
        receivedService = p->srv;
        receivedData = p->data;
        receivedSrc = p->srcAddress;
    });
    handlers.append(handler);
    router1->registerService("echo", handler);

    Packet* packet = new Packet("router-1", "echo", QByteArray("echo-data"));
    packet->srcAddress = "test-client";
    router1->send(packet);

    QTest::qWait(10);

    QCOMPARE(receivedService, QString("echo"));
    QCOMPARE(receivedData, QByteArray("echo-data"));
    QCOMPARE(receivedSrc, QString("test-client"));
}

void TestRouter::testContextIDRouting()
{
    // Test: Response packets should be routed by contextID
    channel1 = new LocalChannel();
    channel2 = channel1->buddy();

    router1->addChannel(channel1);
    router2->addChannel(channel2);

    bool responseReceived = false;
    QByteArray responseData;

    // Register ping service on router2
    auto* pingHandler = new LambdaPacketHandler([&](Packet* p) {
        // Send pong response
        Packet* response = new Packet();
        response->srcAddress = "router-2";
        response->destAddress = p->srcAddress;
        response->ctx = p->ctx;
        response->data = QByteArray("pong!");
        router2->send(response);
    });
    handlers.append(pingHandler);
    router2->registerService("ping", pingHandler);

    // Wait for routes to propagate
    QTest::qWait(100);

    // Register context handler on router1 for response
    auto* contextHandler = new LambdaPacketHandler([&](Packet* p) {
        responseReceived = true;
        responseData = p->data;
    });
    handlers.append(contextHandler);
    uint16_t contextId = router1->registerContextHandler(contextHandler);

    // Send ping request with context ID
    Packet* request = new Packet();
    request->srcAddress = "router-1";
    request->destAddress = "router-2";
    request->srv = "ping";
    request->ctx = contextId;
    request->data = QByteArray("ping?");
    router1->send(request);

    // Wait for round-trip
    QTest::qWait(100);

    QVERIFY(responseReceived);
    QCOMPARE(responseData, QByteArray("pong!"));
}
