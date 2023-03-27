package org.biglittleidea.aln_client;

import org.biglittleidea.aln.IChannel;
import org.biglittleidea.aln.IChannelCloseHandler;
import org.biglittleidea.aln.IPacketHandler;
import org.biglittleidea.aln.Packet;
import org.biglittleidea.aln.Router;
import org.biglittleidea.aln.TcpChannel;

import java.util.concurrent.Semaphore;

public final class ClientExample {

    public static void main(String[] args) {
        try {
            Semaphore lock = new Semaphore(1);
            Router alnRouter = new Router("java-client-1");
            // start the application layer network
            TcpChannel ch = new TcpChannel("localhost", 8081);
            alnRouter.addChannel(ch);

            Thread.sleep(200, 0);

            short ctx = alnRouter.registerContextHandler(new IPacketHandler() {
                @Override
                public void onPacket(Packet p) {
                    System.out.println(String.format("ping response:%s", new String(p.Data)));
                }
            });

            Packet pingPacket = new Packet();
            pingPacket.Service = "ping";
            pingPacket.ContextID = ctx;
            alnRouter.send(pingPacket);

            // release the app on SIGINT
            Runtime.getRuntime().addShutdownHook(new Thread() {
                public void run() {
                    try {
                        System.out.println("Shutting down ...");
                        ch.close();
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        e.printStackTrace();
                    }
                    lock.release(); // let the app exit
                }
            });

            lock.acquire();
            lock.acquire(); // hang until shutdown hook a close condition releases the lock
            lock.release();
        } catch (Exception e) {
            System.out.print(e.getMessage());
        }
    }
}
