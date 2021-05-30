package org.biglittleidea.aln_client;

import org.biglittleidea.aln.IChannel;
import org.biglittleidea.aln.IChannelCloseHandler;
import org.biglittleidea.aln.IPacketHandler;
import org.biglittleidea.aln.Packet;
import org.biglittleidea.aln.TcpChannel;

public final class ClientExample {

    public static void main(String[] args) {
        try {
            TcpChannel ch = new TcpChannel("localhost", 8000);
            ch.send(new Packet());
            ch.receive(new IPacketHandler() {
                public void onPacket(Packet p) {
                    System.out.println("packet received:");
                    System.out.println(p.toString());
                }
            }, new IChannelCloseHandler() {
                public void onChannelClosed(IChannel ch) {
                    System.out.println("channel closed");
                }
            });
            Thread.sleep(10000);
            ch.close();
            Thread.sleep(100);

        } catch (Exception e) {
            System.out.print(e.getMessage());
        }
    }
}
