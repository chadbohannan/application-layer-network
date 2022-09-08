package biglittleidea.aln;

import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.nio.channels.SocketChannel;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import biglittleidea.alnn.App;

public class TcpChannel implements IChannel {
    SocketChannel socket;
    BlockingQueue<Packet> sendQueue = new LinkedBlockingQueue<Packet>(10);
    Thread sendThread = null;
    public TcpChannel(String host, int port) {
        try {
            InetSocketAddress hostAddress = new InetSocketAddress(host, port);
            socket = SocketChannel.open();
            sendThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    try {
                        socket.connect(hostAddress); // block until connection is made
                    } catch (IOException e) {
                        e.printStackTrace();
                        return;
                    }
                    while(true) {
                        Packet p = null;
                        try {
                            p = sendQueue.take();
                            if (p.Net == 0 && p.DestAddress == null && p.Service == null) {
                                socket.close();
                                break;
                            }
                            byte[] frame = p.toFrameBuffer();
                            socket.write(ByteBuffer.wrap(frame));
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                            break;
                        } catch (IOException e) {
                            e.printStackTrace();
                            break;
                        }
                    }
                }
            });
            sendThread.start();
        } catch (Exception e) {
            Log.d("ALNN", String.format("TcpChannel(): %s", e.getLocalizedMessage()));
        }
    }

    public void send(Packet p) {
        sendQueue.offer(p);
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public void receive(IPacketHandler packetHandler, IChannelCloseHandler closeHandler) {
        Parser parser = new Parser(packetHandler);
        if (socket != null && socket.isOpen()) {
            Thread t = new Thread(new Runnable() {
                @Override
                public void run() {
                    while(socket.isOpen()) {
                        if (!socket.isConnected()) {
                            try {
                                Thread.sleep(100);
                            } catch (InterruptedException e) {
                                break;
                            }
                            continue;
                        }
                        ByteBuffer[] buffer = new ByteBuffer[]{ByteBuffer.allocate(1)};
                        try {
                            long n = socket.read(buffer, 0, 1);
                            if (n == 0) {
                                break;
                            }
                            parser.readAx25FrameBytes(buffer[0].array(), (int)n);
                            buffer[0].flip();
                        } catch (IOException e) {
                            e.printStackTrace();
                            break;
                        }
                    }
                    if (closeHandler != null)
                        closeHandler.onChannelClosed(TcpChannel.this);
                }
            });
            t.start();
        }
    }

    public void close() {
        if (socket == null || !socket.isOpen()) {
            return;
        }
        try {
            socket.close();
        } catch (Exception e) {
            Log.d("ALNN", String.format("TcpChannel.close(): %s", e.getLocalizedMessage()));
        }
        sendQueue.offer(new Packet());
    }
}
