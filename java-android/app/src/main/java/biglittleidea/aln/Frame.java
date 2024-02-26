package biglittleidea.aln;

import java.nio.ByteBuffer;

public class Frame {
    public static int BufferSize = 4096;
    public static final byte End = (byte)0xC0;
    public static final byte Esc = (byte)0xDB;
    public static final byte EndT = (byte)0xDC;
    public static final byte EscT = (byte)0xDD;

    // Creates a byte array containing the serialized packet framed for transmission
    public static byte[] toAX25Buffer(byte[] content) {
        ByteBuffer buffer = ByteBuffer.allocate(BufferSize);
        for (int i = 0; i < content.length; i++) {
            Byte b = content[i];
            if (b == End){
                buffer.put(Esc);
                buffer.put(EndT);
            } else if (b == Esc) {
                buffer.put(Esc);
                buffer.put(EscT);
            } else {
                buffer.put(b);
            }
        }
        buffer.put(End);
        byte[] data = new byte[buffer.position()];
        buffer.rewind();
        buffer.get(data);
        return data;

    }

}
