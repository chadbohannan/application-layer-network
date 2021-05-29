package org.biglittleidea.aln;

import org.biglittleidea.aln.Packet;
import org.biglittleidea.aln.Parser;
import org.biglittleidea.aln.IPacketHandler;

import java.util.ArrayList;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

class ParserTest {
    /**
     * Rigorous Test.
     */
    @Test
    void testParser() {
        Packet packet = new Packet();
        packet.NetState = 1;
        packet.SerivceID = 2;
        packet.SourceAddress = 3;
        packet.DestAddress = 4;
        packet.NextAddress = 5;
        packet.SequenceNum = 6;
        packet.AcknowledgeBlock = 7;
        packet.ContextID = 8;
        packet.DataType = 9;
        packet.Data = new byte[]{1,0};
        byte[] buffer = packet.toFrameBuffer();
        
        final ArrayList<Packet> parsedPackets = new ArrayList();
        Parser parser = new Parser(new IPacketHandler(){
            @Override public void onPacket(Packet p) {
                parsedPackets.add(p);
            }
        });
        parser.readBytes(packet.toFrameBuffer());
        assertEquals(1, parsedPackets.size());

        String expected = "cf:0x07fe,net:1,srv:2,src:0x3,dst:0x4,nxt:0x5,seq:6,ack:0x7,ctx:8,typ:9,len:2";
        assertEquals(expected, parsedPackets.remove(0).toString());

    }
}
