package org.biglittleidea.aln;

import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.util.concurrent.TimeUnit;

public class TcpChannel implements IChannel {
    AsynchronousSocketChannel socket;

    public TcpChannel(String host, int port) {
        try {
            InetSocketAddress hostAddress = new InetSocketAddress(host, port);
            socket = AsynchronousSocketChannel.open();
            socket.connect(hostAddress).get(); // block until connection is made
        } catch (Exception e) {
            System.out.println("TcpChannel():" + e.getLocalizedMessage());
        }
    }

    public TcpChannel(AsynchronousSocketChannel socket) {
        this.socket = socket;
    }

    public void send(Packet p) {
        try {
            socket.write(ByteBuffer.wrap(p.toFrameBuffer())).get();
        } catch (Exception e) {
            System.out.println("TcpChannel.send():" + e.getLocalizedMessage());
        }
    }

    public void receive(IPacketHandler packetHandler, IChannelCloseHandler closeHandler) {
        Parser parser = new Parser(packetHandler);
        if (socket != null && socket.isOpen()) {
            int buffSize = 4096;
            ByteBuffer[] buffer = new ByteBuffer[]{ByteBuffer.allocate(buffSize)};
            try {                
                long timeout = 500;
                CompletionHandler<Long,Integer> completionHandler = 
                new CompletionHandler<Long,Integer>(){
                    public void completed(Long result, Integer a){
                        long num = result.longValue();
                        if (num < 0) { // number of bytes read
                            closeHandler.onChannelClosed(TcpChannel.this);
                        } else if (num > 0) {
                            parser.readAx25FrameBytes(buffer[0].array(), (int)num);
                            buffer[0].flip();
                            socket.read(buffer, 0, 1, timeout, TimeUnit.SECONDS, 42, this);
                        }    
                    }
                    public void failed(Throwable exc, Integer a){
                        closeHandler.onChannelClosed(TcpChannel.this);
                    }
                };
                socket.read(buffer, 0, 1, timeout, TimeUnit.MILLISECONDS, 42, completionHandler);
            } catch (Exception e) {
                System.out.println("TcpChannel.recieve():" + e.getLocalizedMessage());
                e.printStackTrace();
            }
        }
    }

    public void close() {
        if (socket == null || !socket.isOpen()) {
            return;
        }
        try {
            socket.close();
        } catch (Exception e) {
            System.out.println("TcpChannel.close()" + e.getLocalizedMessage());
        }
    }
}
