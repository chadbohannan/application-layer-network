package biglittleidea.aln;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothSocket;
import android.content.pm.PackageManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;

import java.io.IOException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class BluetoothChannel implements IChannel {
    BluetoothSocket socket;
    BlockingQueue<Packet> sendQueue = new LinkedBlockingQueue<Packet>(10);
    Thread sendThread = null;

    public BluetoothChannel(BluetoothSocket socket) {
        this.socket = socket;
        try {
            sendThread = new Thread(new Runnable() {
                @SuppressLint("MissingPermission")
                @Override
                public void run() {
                    nap(70);
                    try {
                        socket.connect();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    while(true) {
                        try {
                            Packet p = sendQueue.take();
                            if (p.Net == 0 && p.DestAddress == null && p.Service == null) {
                                socket.close();
                                break;
                            }
                            socket.getOutputStream().write(p.toFrameBuffer());
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

    private void nap(int t){
        try { Thread.sleep(t);}
        catch (InterruptedException e) { }
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    public void receive(IPacketHandler packetHandler, IChannelCloseHandler closeHandler) {
        Parser parser = new Parser(packetHandler);
        if (socket != null) {
            Thread t = new Thread(new Runnable() {
                @Override
                public void run() {
                    while(true) {
                        if (!socket.isConnected()) {
                            nap(100);
                            continue;
                        }
                        byte[] buffer = new byte[]{0};
                        try {
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
                        closeHandler.onChannelClosed(BluetoothChannel.this);
                }
            });
            t.start();
        } else {
            Log.e("ALNN", "receive is bailing without starting");
        }
    }

    public void close() {
        if (socket == null || !socket.isConnected()) {
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
