package org.biglittleidea.aln;

import java.util.ArrayList;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.assertEquals;

class ParserTest {
    /**
     * Rigorous Test.
     */
    @Test
    void testParser() {
        Packet packet = new Packet();
        packet.Net = 1;
        packet.Service = "2";
        packet.SourceAddress = "3";
        packet.DestAddress = "4";
        packet.NextAddress = "5";
        packet.SequenceNum = 6;
        packet.AcknowledgeBlock = 7;
        packet.ContextID = 8;
        packet.DataType = 9;
        packet.Data = new byte[]{1,0};
        
        final ArrayList<Packet> parsedPackets = new ArrayList<Packet>();
        Parser parser = new Parser(new IPacketHandler(){
            @Override public void onPacket(Packet p) {
                parsedPackets.add(p);
            }
        });
        
        byte[] frame = packet.toFrameBuffer();
        parser.readAx25FrameBytes(frame, frame.length);
        assertEquals(1, parsedPackets.size());

        String expected = "cf:0x07fe,net:1,srv:2,src:3,dst:4,nxt:5,seq:6,ack:0x7,ctx:8,typ:9,len:2";
        assertEquals(expected, parsedPackets.get(0).toString());

    }
}
