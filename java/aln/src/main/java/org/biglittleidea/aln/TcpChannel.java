package org.biglittleidea.aln;

import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousSocketChannel;


public class TcpChannel implements IChannel {
    AsynchronousSocketChannel socket;

    public TcpChannel(String host, int port) {
        try{
            InetSocketAddress hostAddress = new InetSocketAddress(host, port);
            socket = AsynchronousSocketChannel.open();
            socket.connect(hostAddress).get(); // block until connection is made
        } catch(Exception e) {
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
    
    public void receive(IPacketHandler ph, IChannelCloseHandler ch){
        Parser parser = new Parser(ph);
        try{
            boolean receiving = true;
            ByteBuffer buffer = ByteBuffer.allocate(4096);
            while (true) {
                buffer.flip();
                int num = socket.read(buffer).get();
                if (num < 1) { // number of bytes read
                    ph.onPacket(new Packet());
                    break;
                };
                parser.readBytes(buffer.array(), num);
            }
        } catch(Exception e) {
            System.out.println(e.getLocalizedMessage());
        }
        ch.onChannelClosed(this);
    }

    public void close(){
        try {
            socket.close();
        } catch (Exception e) {
            System.out.println(e.getLocalizedMessage());
        }
    }
}
