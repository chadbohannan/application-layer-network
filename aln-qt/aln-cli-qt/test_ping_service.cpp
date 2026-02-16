#include "test_ping_service.h"
#include <QSignalSpy>

void TestPingService::init()
{
    serverRouter = new Router("go-host-3f99ea");  // Mimic Go example address
    clientRouter = new Router("client-test");
    channel1 = nullptr;
    channel2 = nullptr;
}

void TestPingService::cleanup()
{
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();

    if (channel1) delete channel1;
    if (channel2) delete channel2;
    if (serverRouter) delete serverRouter;
    if (clientRouter) delete clientRouter;
}

void TestPingService::setupPingService(Router* router)
{
    // Mimic the Go example ping service handler
    auto* handler = new LambdaPacketHandler([router](Packet* packet) {
        qDebug() << "ping from" << packet->srcAddress;

        Packet* response = new Packet();
        response->srcAddress = router->address();
        response->destAddress = packet->srcAddress;
        response->ctx = packet->ctx;
        response->data = QByteArray("pong!");

        router->send(response);
    });
    handlers.append(handler);
    router->registerService("ping", handler);
}

void TestPingService::testPingServiceRegistration()
{
    // Test: Ping service should register successfully
    setupPingService(serverRouter);

    // Send local ping
    bool pingReceived = false;

    auto* contextHandler = new LambdaPacketHandler([&](Packet* p) {
        Q_UNUSED(p);
        pingReceived = true;
    });
    handlers.append(contextHandler);
    uint16_t contextId = serverRouter->registerContextHandler(contextHandler);

    Packet* ping = new Packet();
    ping->srcAddress = serverRouter->address();
    ping->destAddress = serverRouter->address();
    ping->srv = "ping";
    ping->ctx = contextId;
    ping->data = QByteArray("test");

    serverRouter->send(ping);
    QTest::qWait(10);

    QVERIFY(pingReceived);
}

void TestPingService::testPingServiceResponse()
{
    // Test: Ping service should respond with "pong!"
    setupPingService(serverRouter);

    QByteArray responseData;
    bool responseReceived = false;

    auto* contextHandler = new LambdaPacketHandler([&](Packet* p) {
        responseData = p->data;
        responseReceived = true;
    });
    handlers.append(contextHandler);
    uint16_t contextId = serverRouter->registerContextHandler(contextHandler);

    Packet* ping = new Packet();
    ping->srcAddress = serverRouter->address();
    ping->destAddress = serverRouter->address();
    ping->srv = "ping";
    ping->ctx = contextId;
    ping->data = QByteArray("ping");

    serverRouter->send(ping);
    QTest::qWait(10);

    QVERIFY(responseReceived);
    QCOMPARE(responseData, QByteArray("pong!"));
}

void TestPingService::testPingServiceContextID()
{
    // Test: Response should preserve context ID from request
    setupPingService(serverRouter);

    uint16_t receivedContext = 0;
    bool responseReceived = false;

    auto* contextHandler = new LambdaPacketHandler([&](Packet* p) {
        receivedContext = p->ctx;
        responseReceived = true;
    });
    handlers.append(contextHandler);
    uint16_t expectedContext = serverRouter->registerContextHandler(contextHandler);

    Packet* ping = new Packet();
    ping->srcAddress = serverRouter->address();
    ping->destAddress = serverRouter->address();
    ping->srv = "ping";
    ping->ctx = expectedContext;
    ping->data = QByteArray("ping");

    serverRouter->send(ping);
    QTest::qWait(10);

    QVERIFY(responseReceived);
    QCOMPARE(receivedContext, expectedContext);
}

void TestPingService::testMultiplePingRequests()
{
    // Test: Service should handle multiple ping requests
    setupPingService(serverRouter);

    int responseCount = 0;
    QList<uint16_t> receivedContexts;

    auto responseFunc = [&](Packet* p) {
        responseCount++;
        receivedContexts.append(p->ctx);
    };

    auto* handler1 = new LambdaPacketHandler(responseFunc);
    auto* handler2 = new LambdaPacketHandler(responseFunc);
    auto* handler3 = new LambdaPacketHandler(responseFunc);
    handlers.append(handler1);
    handlers.append(handler2);
    handlers.append(handler3);

    uint16_t ctx1 = serverRouter->registerContextHandler(handler1);
    uint16_t ctx2 = serverRouter->registerContextHandler(handler2);
    uint16_t ctx3 = serverRouter->registerContextHandler(handler3);

    // Send 3 ping requests
    QList<uint16_t> contextIds = {ctx1, ctx2, ctx3};
    for (uint16_t ctxId : contextIds) {
        Packet* ping = new Packet();
        ping->srcAddress = serverRouter->address();
        ping->destAddress = serverRouter->address();
        ping->srv = "ping";
        ping->ctx = ctxId;
        ping->data = QByteArray("ping");
        serverRouter->send(ping);
    }

    QTest::qWait(50);

    QCOMPARE(responseCount, 3);
    QVERIFY(receivedContexts.contains(ctx1));
    QVERIFY(receivedContexts.contains(ctx2));
    QVERIFY(receivedContexts.contains(ctx3));
}

void TestPingService::testTwoRoutersPingPong()
{
    // Test: Ping-pong between two routers (mimics real network scenario)
    // This is like the Go example with a TCP connection between two nodes

    // Set up ping service on server
    setupPingService(serverRouter);

    // Connect routers via local channel (simulates TCP connection)
    channel1 = new LocalChannel();
    channel2 = channel1->buddy();

    serverRouter->addChannel(channel1);
    clientRouter->addChannel(channel2);

    // Wait for route advertisements
    QTest::qWait(100);

    // Client sends ping to server
    bool clientGotPong = false;
    QByteArray pongData;

    auto* contextHandler = new LambdaPacketHandler([&](Packet* p) {
        clientGotPong = true;
        pongData = p->data;
    });
    handlers.append(contextHandler);
    uint16_t contextId = clientRouter->registerContextHandler(contextHandler);

    Packet* ping = new Packet();
    ping->srcAddress = clientRouter->address();
    ping->destAddress = serverRouter->address();
    ping->srv = "ping";
    ping->ctx = contextId;
    ping->data = QByteArray("ping from client");

    clientRouter->send(ping);

    // Wait for round-trip
    QTest::qWait(100);

    QVERIFY(clientGotPong);
    QCOMPARE(pongData, QByteArray("pong!"));
}
