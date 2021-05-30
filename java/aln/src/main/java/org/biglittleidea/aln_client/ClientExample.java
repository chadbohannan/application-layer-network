package org.biglittleidea.aln_client;

import org.biglittleidea.aln.IChannel;
import org.biglittleidea.aln.IChannelCloseHandler;
import org.biglittleidea.aln.IPacketHandler;
import org.biglittleidea.aln.Packet;
import org.biglittleidea.aln.TcpChannel;
/**
 * Hello world!
 */
public final class ClientExample {
    private ClientExample() {
    }

    /**
     * Says hello to the world.
     * @param args The arguments of the program.
     */
    public static void main(String[] args) {
        try{
            TcpChannel ch = new TcpChannel("localhost", 8000);
            ch.send(new Packet());
            ch.receive(new IPacketHandler(){
                public void onPacket(Packet p){
                    System.out.println("packet received");
                }
            }, new IChannelCloseHandler(){
                public void onChannelClosed(IChannel ch){
                    System.out.println("channel closed");
                }                
            });
            Thread.sleep(1000);
            ch.close();
            Thread.sleep(1000);
            
        } catch(Exception e) {
            System.out.print(e.getMessage());
        }
    }
}
