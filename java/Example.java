import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;


public class Example {

    public static void main(String[] args) {
        Parser.IPacketHandler handler = new Parser.IPacketHandler(){
            public void onPacket(Packet p) {
                for (int i = 0; i < p.Data.length; i++)
                    System.out.print((char) p.Data[i]);
                System.out.printf("\n%d data bytes, crc=%x\n", p.Data.length, p.CRC);
            }
        };

        Parser parser = new Parser(handler);
        File file = new File("packetstream.input");
        
        try {
            FileInputStream is = new FileInputStream(file);
            byte[] b = new byte[5];
            int l = is.read(b);
            while (l > 0) {
                parser.readBytes(b);
                l = is.read(b);
            }
            is.close();
        } catch(Exception e){
            System.out.println(e.getMessage());
        }

    }

}