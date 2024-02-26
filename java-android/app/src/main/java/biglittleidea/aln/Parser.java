package biglittleidea.aln;

import java.nio.ByteBuffer;

public class Parser {

    private class State {
        public static final int Buffering = 0;
        public static final int Escaped = 1;
    }

    private int state;
    ByteBuffer buffer; // byte buffer a Packet is parsed from
    
    IPacketHandler handler;

    public Parser(IPacketHandler handler) {
        this.handler = handler;
        buffer = ByteBuffer.allocate(Frame.BufferSize);
        state = State.Buffering;
    }

    public void reset() {
        state = State.Buffering;
        buffer.clear();
    }

    private void acceptPacket() {
        handler.onPacket(new Packet(buffer.array()));
        reset();
    }

    public void readAx25FrameBytes(byte[] data, int length){
        for( int i = 0; i < length; i++){
            if (data[i] == Frame.End){
                acceptPacket();
            } else if (state == State.Escaped) {
                if (data[i] == Frame.EndT) {
                    buffer.put(Frame.End);
                }else if (data[i] == Frame.EscT) {
                    buffer.put(Frame.Esc);
                }
                state = State.Buffering;
            } else if (data[i] == Frame.Esc) {
                state = State.Escaped;
            } else {
                try {
                buffer.put(data[i]);
                } catch(Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

}
