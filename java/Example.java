import java.util.ArrayList;

public class Example {

    public static void main(String[] args) {
        // Prints "Hello, World" to the terminal window.
        System.out.println("Hello, World");

        Packet p1 = new Packet();
        p1.DestinationAddress = 43;
        
        ArrayList<Byte> byteList = p1.toPacketBuffer();
        System.out.println(byteList);

        Packet p2 = new Packet(byteList);
        System.out.println(p2.toPacketBuffer());

        byte[] bytes = new byte[]{
            (byte) 0x7F,
            (byte) 0xFF,
            (byte) 0xFF,
            (byte) 0xFF
        };

        int v1 = Packet.readUINT32(bytes, 0);
        System.out.println(v1);

        byte[] a = Packet.writeUINT32(v1);
        System.out.println(a);

        int v2 = Packet.readUINT32(a, 0);
        System.out.println(v2);

    }

}