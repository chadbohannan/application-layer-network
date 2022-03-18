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
            System.out.println(e.getLocalizedMessage());
        }
    }

    public void send(Packet p) {
        try {
            socket.write(ByteBuffer.wrap(p.toFrameBuffer())).get();
        } catch (Exception e) {
            System.out.println(e.getLocalizedMessage());
        }
    }

    public void receive(IPacketHandler packetHandler, IChannelCloseHandler closeHandler) {
        Parser parser = new Parser(packetHandler);
        if (socket != null && socket.isOpen()) {
            int buffSize = 4096;
            ByteBuffer[] buffer = new ByteBuffer[]{ByteBuffer.allocate(buffSize)};
            try {                
                int offset = 0;
                long timeout = 500;
                CompletionHandler completionHandler = new CompletionHandler<Long,Integer>(){
                    public void completed(Long result, Integer a){
                        long num = result.longValue();
                        if (num < 0) { // number of bytes read
                            closeHandler.onChannelClosed(TcpChannel.this);
                        } else if (num > 0) {
                            parser.readBytes(buffer[0].array(), (int)num);
                            buffer[0].flip();
                            socket.read(buffer, 0, 1, timeout, TimeUnit.SECONDS, 42, this);
                        }    
                    }
                    public void failed(Throwable exc, Integer a){
                        //System.out.println(exc.getLocalizedMessage());
                    }
                };
                socket.read(buffer, 0, 1, timeout, TimeUnit.MILLISECONDS, 42, completionHandler);
            } catch (Exception e) {
                System.out.println(e.getLocalizedMessage());
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
            System.out.println(e.getLocalizedMessage());
        }
    }
}