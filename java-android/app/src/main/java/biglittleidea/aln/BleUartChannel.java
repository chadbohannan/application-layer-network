package biglittleidea.aln;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.util.Log;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.UUID;

import biglittleidea.alnn.App;

@SuppressLint("MissingPermission")
public class BleUartChannel implements IChannel {
    IChannelCloseHandler ch = null;
    IPacketHandler ph = null;

    Parser alnPacketParser = null;
    BleUartSerial bleDevice;
    public BleUartChannel(BleUartSerial bleDevice) {
        this.bleDevice = bleDevice;
    }

    @Override
    public void send(Packet p) {
        byte[] frame = Frame.toAX25Buffer(p.toFrameBuffer());
        this.bleDevice.send(frame);
    }

    @Override
    public void receive(IPacketHandler ph, IChannelCloseHandler ch) {
        this.ph = ph;
        this.ch = ch;
        this.alnPacketParser = new Parser(ph);
        bleDevice.setOnDataHandler(new BleUartSerial.OnDataHandler() {
            @Override
            public void onData(byte[] data) {
                alnPacketParser.readAx25FrameBytes(data, data.length);
            }
        });
        bleDevice.setOnConnectHandler(isConnected -> {
            if (!isConnected) {
                close();
            }
        });
    }

    @Override
    public void close() {
        if (this.ch != null) {
            bleDevice.close();
            this.ch.onChannelClosed(this);
            this.ch = null;
        }
    }
}
