package org.biglittleidea.aln;


import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

class RouterTest {
    @Test
    void testLocalRoute() {
        LocalChannel lc = new LocalChannel();
        LocalChannel lb = lc.LoopBack();

        Router router1 = new Router("r1");
        Router router2 = new Router("r2");
        
        Semaphore lock = new Semaphore(1);
        lock.tryAcquire();
        final Packet[] recvdPkt = new Packet[]{null};
        router2.registerService("test", new IPacketHandler(){
            @Override
            public void onPacket(Packet p) {
                recvdPkt[0] = p;                
                lock.release();
            }});

        router1.addChannel(lc);
        router2.addChannel(lb);
        
        Packet packet = new Packet();
        packet.Service = "test";
        router1.send(packet);
        
        try {
            lock.tryAcquire(500, TimeUnit.MILLISECONDS);
            lock.release();
        } catch (InterruptedException e) {
            System.out.print("router.send timed out");
            assertNull(e);
        }
        
        assertNotNull(recvdPkt[0]);
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
        Packet packet = r.composeNetServiceShare("node-b", "ping", (short)2);

        Router.ServiceNodeInfo parsedInfo = r.parseNetServiceShare(packet);
        assertNull(parsedInfo.err);
        assertEquals("node-b", parsedInfo.address);
        assertEquals("ping", parsedInfo.service);
        assertEquals((short)2, parsedInfo.load);
    }

}
