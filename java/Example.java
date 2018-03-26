
public class Example {

    public static void main(String[] args) {
        // Prints "Hello, World" to the terminal window.
        System.out.println("Hello, World");

        Packet packet = new Packet();
        packet.DestAdress = 0x42;
        
        System.out.println(packet.toPacketBuffer());
    }

}