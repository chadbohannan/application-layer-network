package org.biglittleidea.aln_host;

import org.biglittleidea.aln.IChannel;
import org.biglittleidea.aln.IChannelCloseHandler;
import org.biglittleidea.aln.IPacketHandler;
import org.biglittleidea.aln.Packet;
import org.biglittleidea.aln.Router;
import org.biglittleidea.aln.TcpChannel;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.channels.AsynchronousServerSocketChannel;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.util.concurrent.Semaphore;

public final class HostExample {

    public static void main(String[] args) {
        try {
            Semaphore lock = new Semaphore(1);
            Router alnRouter = new Router("java-host-1");

            alnRouter.registerService("ping", new IPacketHandler() {
                @Override
                public void onPacket(Packet p) {
                    System.out.println("ping from " + p.SourceAddress);
                    Packet response = new Packet();
                    response.DestAddress  = p.SourceAddress;
                    response.ContextID = p.ContextID;
                    response.Data = "pong".getBytes();
                    alnRouter.send(response);
                }
            });

            int port = 8081;
            final AsynchronousServerSocketChannel listener =
                    AsynchronousServerSocketChannel.open().bind(new InetSocketAddress(port));

            listener.accept( null, new CompletionHandler<AsynchronousSocketChannel,Void>() {

                @Override
                public void completed(AsynchronousSocketChannel ch, Void att) {
                    // Accept the next connection
                    listener.accept( null, this );
                    alnRouter.addChannel(new TcpChannel(ch));
                    System.out.println("accepted channel");
                }

                @Override
                public void failed(Throwable arg0, Void arg1) {
                    System.out.println("listen failed");
                }
            });

            // release the app on SIGINT
            Runtime.getRuntime().addShutdownHook(new Thread() {
                public void run() {
                    try {
                        System.out.println("Shutting down ...");
                        listener.close();
                        Thread.sleep(100);
                    } catch (InterruptedException | IOException e) {
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
