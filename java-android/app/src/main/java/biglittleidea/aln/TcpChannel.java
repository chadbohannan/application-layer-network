package biglittleidea.aln;

import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.io.IOException;
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

import biglittleidea.alnn.App;

public class TcpChannel implements IChannel {
    SocketChannel socket;
    BlockingQueue<Packet> sendQueue = new LinkedBlockingQueue<Packet>(10);
    Thread sendThread = null;
    Thread receiveThread = null;

    // TODO onCloseHandler to notify UI when channel closes?
    IChannelCloseHandler closeHandler = null;

    public TcpChannel(SocketChannel socket) {
        try {
            socket.finishConnect();
        } catch (IOException e) {
            Log.e("ALNN", "TcpChannel finishConnect err:" + e.getMessage());
        }
        this.socket = socket;
        sendThread = new Thread(() -> {
            sendLoop();
            triggerOnCloseHandler();
        });
        sendThread.start();
    }

    public TcpChannel(String host, int port) {
        try {
            receiveThread = new Thread(() -> {

                try {
                    socket = SocketChannel.open();
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
                sendThread = new Thread(() -> {
                    try {
                        InetSocketAddress hostAddress = new InetSocketAddress(host, port);
                        socket.connect(hostAddress); // block until connection is made
                    } catch (IOException e) {
                        e.printStackTrace();
                        return;
                    }
                    sendLoop();
                    triggerOnCloseHandler();

                });
                sendThread.start();
            });
            receiveThread.start();
        } catch (Exception e) {
            Log.d("ALNN", String.format("TcpChannel(): %s", e.getLocalizedMessage()));
        }
    }

    private void sendLoop() {
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

    public void send(Packet p) {
        sendQueue.offer(p);
    }

    private boolean readSocket(SocketChannel socket, Parser parser) {
        ByteBuffer[] buffer = new ByteBuffer[]{ByteBuffer.allocate(1)};
        try {
            long n = socket.read(buffer, 0, 1);
            if (n == 0) {
                return true;
            }
            if (n < 0) {
                Thread.sleep(10);
            } else {
                parser.readAx25FrameBytes(buffer[0].array(), (int) n);
                buffer[0].flip();
            }
        } catch (IOException e) {
            e.printStackTrace();
            return true;
        } catch (InterruptedException e) {
            e.printStackTrace();
            return true;
        }
        return false;
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public void receive(IPacketHandler packetHandler, IChannelCloseHandler closeHandler) {
        this.closeHandler = closeHandler;
        Parser parser = new Parser(packetHandler);
        new Thread(new Runnable() {
            @Override
            public void run() {
                // wait for init thread to connect
                for (int i = 0; i < 5; i++) {
                    if (socket == null || !socket.isOpen()) {
                        try {
                            Thread.sleep(300);
                        } catch (InterruptedException e) {
                            break;
                        }
                    }
                }
                while(socket != null && socket.isOpen()) {
                    if (!socket.isConnected()) {
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException e) {
                            break;
                        }
                        continue;
                    }
                    if (readSocket(socket, parser))
                        break;
                }
                triggerOnCloseHandler();
            }
        }).start();
    }

    public void close() {
        if (socket == null || !socket.isOpen()) {
            return;
        }
        try {
            socket.close();
            triggerOnCloseHandler();
        } catch (Exception e) {
            Log.d("ALNN", String.format("TcpChannel.close(): %s", e.getLocalizedMessage()));
        }
        sendQueue.offer(new Packet());
    }

    private void triggerOnCloseHandler() {
        if (closeHandler != null)
            closeHandler.onChannelClosed(TcpChannel.this);
        closeHandler = null;
    }
}
