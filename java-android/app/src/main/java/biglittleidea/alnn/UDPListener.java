package biglittleidea.alnn;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.util.Log;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;

public class UDPListener extends Thread {

    public interface MessageHandler {
        void onMessage(byte[] message);
    }
    DatagramSocket udpSocket = null;
    MessageHandler handler = null;
    boolean running = false;
    InetAddress addr;
    short port;
    public UDPListener(InetAddress addr, short port) {
        this.addr = addr;
        this.port = port;
    }

    public void setMessageHandler(MessageHandler msgHandler) {
        handler = msgHandler;
    }

    @Override
    public void run() {
        if (running) return;
        running = true;
        Context ctx = App.getInstance().getApplicationContext();
        WifiManager wifi = (WifiManager) ctx.getSystemService(Context.WIFI_SERVICE);
        WifiManager.MulticastLock mcLock = wifi.createMulticastLock("org.biglittleidea.alnn");
        try {
            mcLock.acquire();

            udpSocket = new DatagramSocket(null);
            udpSocket.setReuseAddress(true);
            udpSocket.setBroadcast(true);
//            udpSocket.setSoTimeout(2000);
//            udpSocket.connect(addr, port);
            udpSocket.bind(new InetSocketAddress(addr, port));

            while (running) {
                try {
                    byte[] buff = new byte[4096];
                    DatagramPacket packet = new DatagramPacket(buff, buff.length);
                    udpSocket.receive(packet);
                    if (handler != null) {
                        byte[] msg = new byte[packet.getLength()];
                        for(int i = 0; i < msg.length; i++) {
                            msg[i] = buff[i];
                        }
                        handler.onMessage(msg);
                    }
                    String text = new String(buff, 0, packet.getLength());
                    Log.d("ALNN", "Received text: " + text);
                } catch (SocketTimeoutException e) {
                    // it's fine
                    Log.d("ALNN", " UDP SocketTimeout");
                } catch (IOException e) {
                    Log.e("ALNN", " UDP IOException", e);
                    running = false;
                    udpSocket.close();
                }
            }
        } catch (SocketException e) {
            Log.e("ALNN", "SocketExeption:", e);
        } catch (Exception e) {
            Log.e("ALNN", "UDP Listen Error:", e);
        }
        end();
        mcLock.release();
        running = false;
    }

    public void end() {
        running = false;
        if(udpSocket != null){
            udpSocket.close();
        }
    }

    public boolean isRunning() {
        return running;
    }
}