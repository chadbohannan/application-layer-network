package org.biglittleidea.aln;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;

import java.util.concurrent.Semaphore;

class RouterTest {
    @Test
    void testLocalOneHopRoute() {

        Router router1 = new Router("r1");
        Router router2 = new Router("r2");

        Semaphore lock = new Semaphore(1);
        lock.tryAcquire();
        final Packet[] recvdPkt = new Packet[] { null };
        router2.registerService("test", new IPacketHandler() {
            @Override
            public void onPacket(Packet p) {
                recvdPkt[0] = p;
                lock.release();
            }
        });

        LocalChannel lc = new LocalChannel();
        LocalChannel lb = lc.LoopBack();
        router1.addChannel(lc);
        router2.addChannel(lb);

        Packet packet = new Packet();
        packet.Service = "test";
        packet.AcknowledgeBlock = 42;
        router1.send(packet);

        assertNotNull(recvdPkt[0]);
        assertEquals(42, recvdPkt[0].AcknowledgeBlock);
    }

    @Test
    void testLocalTwoHopRoute() {
        Router router1 = new Router("r1");
        Router router2 = new Router("r2");
        Router router3 = new Router("r3");

        Semaphore lock = new Semaphore(1);
        lock.tryAcquire();
        final Packet[] recvdPkt = new Packet[] { null };
        router3.registerService("test", new IPacketHandler() {
            @Override
            public void onPacket(Packet p) {
                recvdPkt[0] = p;
                lock.release();
            }
        });

        LocalChannel lc1 = new LocalChannel();
        LocalChannel lb1 = lc1.LoopBack();
        router1.addChannel(lc1);
        router2.addChannel(lb1);

        LocalChannel lc2 = new LocalChannel();
        LocalChannel lb2 = lc2.LoopBack();
        router2.addChannel(lc2);
        router3.addChannel(lb2);

        Packet packet = new Packet();
        packet.Service = "test";
        packet.AcknowledgeBlock = 42;
        router1.send(packet);

        assertNotNull(recvdPkt[0]);
        assertEquals(42, recvdPkt[0].AcknowledgeBlock);
    }

    @Test
    void testChannelClosedCleanup() {
        Router router1 = new Router("r1");
        Router router2 = new Router("r2");
        LocalChannel lc = new LocalChannel();
        LocalChannel lb = lc.LoopBack();
        router1.addChannel(lc);
        router2.addChannel(lb);
        assertEquals(1, router1.channels.size());
        assertEquals(1, router1.remoteNodeMap.size());
        lc.close();
        assertEquals(0, router1.channels.size());
        assertEquals(0, router1.remoteNodeMap.size());
    }

    @Test
    void testNetShareParser() {
        Router r = new Router("node-a");
        String address = "node-b";
        short cost = 3;
        Packet packet = r.composeNetRouteShare(address, cost);
        Router.RemoteNodeInfo parsedInfo = r.parseNetRouteShare(packet);
        assertNull(parsedInfo.err);
        assertEquals(address, parsedInfo.address);
        assertEquals(cost, parsedInfo.cost);
        assertEquals(r.address, parsedInfo.nextHop);
    }

    @Test
    void testNetServiceShare() {
        Router r = new Router("node-a");
        Packet packet = r.composeNetServiceShare("node-b", "ping", (short) 2);

        Router.ServiceNodeInfo parsedInfo = r.parseNetServiceShare(packet);
        assertNull(parsedInfo.err);
        assertEquals("node-b", parsedInfo.address);
        assertEquals("ping", parsedInfo.service);
        assertEquals((short) 2, parsedInfo.load);
    }

}
