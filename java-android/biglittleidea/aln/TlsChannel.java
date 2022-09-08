package biglittleidea.aln;

import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.channels.AsynchronousSocketChannel;
import java.nio.channels.CompletionHandler;
import java.nio.channels.SocketChannel;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.SSLSocketFactory;

import biglittleidea.alnn.App;

public class TlsChannel implements IChannel {
    Socket socket;
    BlockingQueue<Packet> sendQueue = new LinkedBlockingQueue<Packet>(10);
    Thread sendThread = null;
    IPacketHandler packetHandler = null;
    IChannelCloseHandler closeHandler = null;
    boolean isRunning = false;

    public TlsChannel(String host, int port) {
        try {
            sendThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    try {
                        socket = SSLSocketFactory.getDefault().createSocket(host, port);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    if (socket == null) {
                        return;
                    }
                    synchronized (this) {
                        if (packetHandler != null && !isRunning) {
                            makeRecieverThread().start();
                            isRunning = true;
                        }
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
                            OutputStream os = socket.getOutputStream();
                            os.write(frame);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                            break;
                        } catch (IOException e) {
                            e.printStackTrace();
                            break;
                        } catch (Exception e) {
                            e.printStackTrace();
                            break;
                        }
                    }
                    if (!socket.isClosed()) {
                        try {
                            socket.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            });
            sendThread.start();
        } catch (Exception e) {
            Log.d("ALNN", String.format("TlsChannel(): %s", e.getLocalizedMessage()));
        }
    }

    public void send(Packet p) {
        sendQueue.offer(p);
    }

    protected Thread makeRecieverThread() {
        return new Thread(new Runnable() {
            @Override
            public void run() {
                Parser parser = new Parser(packetHandler);
                while(!socket.isClosed()) {
                    if (!socket.isConnected()) {
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException e) {
                            break;
                        }
                        continue;
                    }
                    try {
                        byte[] buffer = new byte[1];
                        long n = socket.getInputStream().read(buffer, 0, 1);
                        if (n == 0) {
                            break;
                        }
                        parser.readAx25FrameBytes(buffer, (int)n);
                    } catch (IOException e) {
                        e.printStackTrace();
                        break;
                    }
                }
                if (closeHandler != null)
                    closeHandler.onChannelClosed(TlsChannel.this);

                if (!socket.isClosed()) {
                    try {
                        socket.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        });
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public void receive(IPacketHandler packetHandler, IChannelCloseHandler closeHandler) {
        this.packetHandler = packetHandler;
        this.closeHandler = closeHandler;
        if (socket != null && !socket.isClosed()) {
            synchronized (this) {
                if (!isRunning) {
                    makeRecieverThread().start();
                    isRunning = true;
                }
            }
        }
    }

    public void close() {
        if (socket == null || socket.isClosed()) {
            return;
        }
        try {
            socket.close();
        } catch (Exception e) {
            Log.d("ALNN", String.format("TslChannel.close(): %s", e.getLocalizedMessage()));
        }
        sendQueue.offer(new Packet());
    }
}
