package org.biglittleidea.aln;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;

/**
 * Unit test for simple App.
 */
class PacketTest {
    /**
     * Rigorous Test.
     */
    @Test
    void testPacketBuffer() {
        Packet packet = new Packet();
        packet.NetState = 1;
        packet.SerivceID = 2;
        packet.SourceAddress = "3";
        packet.DestAddress = "4";
        packet.NextAddress = "5";
        packet.SequenceNum = 6;
        packet.AcknowledgeBlock = 7;
        packet.ContextID = 8;
        packet.DataType = 9;
        packet.Data = new byte[]{1,0};
        byte[] buffer = packet.toPacketBuffer();
        Packet newPacket = new Packet(buffer);
        String expected = "cf:0x07fe,net:1,srv:2,src:3,dst:4,nxt:5,seq:6,ack:0x7,ctx:8,typ:9,len:2";
        assertEquals(expected, newPacket.toString());
    }
}
