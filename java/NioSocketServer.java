// https://github.com/55pp/testTravis/blob/master/clientD/src/main/java/com/NIOSocketServer.java
// sets up an example asynchronous socket server

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

public class NioSocketServer
{
    public NioSocketServer(int port)
    {
        try{
            final AsynchronousServerSocketChannel listener =
                    AsynchronousServerSocketChannel.open().bind(new InetSocketAddress(port));

            listener.accept( null, new CompletionHandler<AsynchronousSocketChannel,Void>() {

                @Override
                public void completed(AsynchronousSocketChannel ch, Void att) {
                    // Accept the next connection
                    listener.accept( null, this );

                    // Greet the client
                    ch.write( ByteBuffer.wrap( "Hello, I am Echo Server 2020, let's have an engaging conversation!\n".getBytes() ) );

                    // Allocate a byte buffer (4K) to read from the client
                    ByteBuffer byteBuffer = ByteBuffer.allocate( 4096 );
                    try{
                        boolean running = true;
                        int bytesRead = ch.read( byteBuffer ).get( 20, TimeUnit.SECONDS ); // Read the first line
                        while( bytesRead != -1 && running ) {
                            System.out.println( "bytes read: " + bytesRead );

                            // Make sure that we have data to read
                            if( byteBuffer.position() > 2 ){
                                byteBuffer.flip(); // Make the buffer ready to read

                                // Convert the buffer into a line
                                byte[] lineBytes = new byte[ bytesRead ];
                                byteBuffer.get( lineBytes, 0, bytesRead );
                                String line = new String( lineBytes );

                                
                                System.out.println( "Message: " + line ); // Debug

                                // Echo back to the caller
                                ch.write( ByteBuffer.wrap( line.getBytes() ) );

                                byteBuffer.clear(); // Make the buffer ready to write

                                // Read the next line
                                bytesRead = ch.read( byteBuffer ).get( 20, TimeUnit.SECONDS );
                            }
                            else
                            {
                                // An empty line signifies the end of the conversation in our protocol
                                running = false;
                            }
                        }
                    }
                    catch (InterruptedException e)
                    {
                        e.printStackTrace();
                    }
                    catch (ExecutionException e)
                    {
                        e.printStackTrace();
                    }
                    catch (TimeoutException e)
                    {
                        // The user exceeded the 20 second timeout, so close the connection
                        ch.write( ByteBuffer.wrap( "Good Bye\n".getBytes() ) );
                        System.out.println( "Connection timed out, closing connection" );
                    }

                    System.out.println( "End of conversation" );
                    try
                    {
                        // Close the connection if we need to
                        if( ch.isOpen() )
                        {
                            ch.close();
                        }
                    }
                    catch (IOException e1)
                    {
                        e1.printStackTrace();
                    }
                }

                @Override
                public void failed(Throwable exc, Void att) {
                    ///...
                }
            });
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }
    }

    public static void main( String[] args )
    {
        NioSocketServer server = new NioSocketServer(8080);
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