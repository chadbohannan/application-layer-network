// https://github.com/55pp/testTravis/blob/master/clientD/src/main/java/com/NioSocketClient.java
// sets up an example asynchronous socket client
package com.geekcap.javaworld.nio2;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousServerSocketChannel;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.Future;

public class NioSocketClient
{
    public String sendMessage(AsynchronousSocketChannel client, String message) {
        
        // send message
        byte[] byteMsg = new String(message).getBytes();
        ByteBuffer buffer = ByteBuffer.wrap(byteMsg);
        Future<Integer> writeResult = client.write(buffer);
        writeResult.get(); // number of bytes writen

        // read response
        buffer.flip();
        Future<Integer> readResult = client.read(buffer);
        readResult.get(); // number of bytes read

        String echo = new String(buffer.array()).trim();
        buffer.clear();
        return echo;
    }

    public NioSocketClient(String host, int port) {
        AsynchronousSocketChannel client = AsynchronousSocketChannel.open();
        InetSocketAddress hostAddress = new InetSocketAddress(host, port);
        //Future<Void> future = client.connect(hostAddress);

        CompletionHandler completionHandler = new CompletionHandler<AsynchronousSocketChannel,Void>() {

                @Override
                public void completed(AsynchronousSocketChannel ch, Void att) {
                    sendMessage(ch, "hi");
                }
        };

        Future<Void> future = client.connect(hostAddress, null, completionHandler);
        // future.get();
    }

    public static void main( String[] args )
    {
        NioSocketClient server = new NioSocketClient(8080);
        try
        {
            Thread.sleep( 60000 );
        }
        catch( Exception e )
        {
            e.printStackTrace();
        }
    }
}