package org.biglittleidea.maln_client;

import java.util.concurrent.Semaphore;
import java.util.Date;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.TimeZone;
import java.util.UUID;
import org.biglittleidea.aln.IPacketHandler;
import org.biglittleidea.aln.Packet;
import org.biglittleidea.aln.Router;
import org.biglittleidea.aln.TcpChannel;

/* TODO docs */


public final class MalnClientExample {
    // decomposed URL from tcp+maln://layer7node.net:8000/6b404c2d-50d1-4007-94af-1b157c64e4e3
    public static String host = "layer7node.net";
    public static int port = 8000;
    public static String alnNode = "6b404c2d-50d1-4007-94af-1b157c64e4e3";

    public static void main(String[] args) {
        try {
            UUID uuid = UUID.randomUUID();
            String localAddress = "java-client-" + uuid.toString();
            System.out.println("localAddress: " + localAddress);

            Router alnRouter = new Router(localAddress);
            // start the application layer network
            TcpChannel ch = new TcpChannel( host, port);

            Packet alnSelect = new Packet();
            alnSelect.DestAddress = alnNode;
            ch.send(alnSelect);

            // let the router manage the channel from this point
            alnRouter.addChannel(ch);

            TimeZone tz = TimeZone.getTimeZone("UTC");
            DateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm'Z'"); // Quoted "Z" to indicate UTC, no timezone offset
            df.setTimeZone(tz);

            while (true) {
                Thread.sleep(5000, 0);
                Packet timePacket = new Packet();
                timePacket.Service = "log";
                timePacket.Data = df.format(new Date()).getBytes();
                alnRouter.send(timePacket);
            }
        } catch (Exception e) {
            System.out.print(e.getMessage());
        }
    }
}
