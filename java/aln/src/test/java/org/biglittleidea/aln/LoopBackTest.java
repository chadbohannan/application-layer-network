package org.biglittleidea.aln;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

class LoopBackTest {
    @Test
    void testLoopBackSend() {
        Packet packet = new Packet();
        packet.ContextID = 1;
        packet.DataType = 9;
        packet.Data = new byte[]{1,0};
        
        final Packet[] recvdPkt = new Packet[]{null};
        LocalChannel lc = new LocalChannel();
        LocalChannel lb = lc.LoopBack();
        lb.receive(new IPacketHandler(){
            @Override
            public void onPacket(Packet p) {
                recvdPkt[0] = p;
                
            }}, new IChannelCloseHandler() {
                @Override
                public void onChannelClosed(IChannel c) { }
            }
        );
        lc.send(packet);
        assertNotNull(recvdPkt[0]);
        assertEquals(9, recvdPkt[0].DataType);
    }

    @Test
    void testLoopBackRecieve() {
        Packet packet = new Packet();
        packet.ContextID = 1;
        packet.DataType = 9;
        packet.Data = new byte[]{1,0};
        
        final Packet[] recvdPkt = new Packet[]{null};
        LocalChannel lc = new LocalChannel();
        LocalChannel lb = lc.LoopBack();
        lc.receive(new IPacketHandler(){
            @Override
            public void onPacket(Packet p) {
                recvdPkt[0] = p;
                
            }}, new IChannelCloseHandler() {
                @Override
                public void onChannelClosed(IChannel c) { }
            }
        );
        lb.send(packet);
        assertNotNull(recvdPkt[0]);
        assertEquals(9, recvdPkt[0].DataType);
    }
}
