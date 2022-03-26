package org.biglittleidea.aln;

import java.nio.ByteBuffer;

public class Frame {
    public static int BufferSize = 4096;
    public static final byte End = (byte)0XC0;
    public static final byte Esc = (byte)0xDB;
    public static final byte EndT = (byte)0xDC;
    public static final byte EscT = (byte)0xDD;

        // Creates a byte array containing the serialized packet framed for transmission
        public static byte[] toAX25Buffer(byte[] content) {
            ByteBuffer byteBuffer = ByteBuffer.allocate(BufferSize);
            for (int i = 0; i < content.length; i++) {
                Byte b = content[i];
                if (b == End){
                    byteBuffer.put(Esc);
                    byteBuffer.put(EndT);
                } else if (b == Esc) {
                    byteBuffer.put(Esc);
                    byteBuffer.put(EscT);
                } else {
                    byteBuffer.put(b);
                }
            }
            byteBuffer.put(End);
            return byteBuffer.array();
        }

        public static byte[] fromAX25Buffer(byte[] content) {
            ByteBuffer byteBuffer = ByteBuffer.allocate(content.length);
            // TODO
            return byteBuffer.array();
        }
}
