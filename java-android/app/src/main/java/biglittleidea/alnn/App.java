package biglittleidea.alnn;

import android.annotation.SuppressLint;
import android.app.Application;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Looper;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.lifecycle.MutableLiveData;

import com.journeyapps.barcodescanner.ScanOptions;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.UUID;

import biglittleidea.aln.BleUartChannel;
import biglittleidea.aln.BleUartSerial;
import biglittleidea.aln.BluetoothChannel;
import biglittleidea.aln.IChannel;
import biglittleidea.aln.Router;
import biglittleidea.aln.TcpChannel;
import biglittleidea.aln.Packet;
import biglittleidea.aln.TlsChannel;

public class App extends Application {
    private static App instance;
    public ActivityResultLauncher<ScanOptions> fragmentLauncher;

    List<LocalServiceHandler> localServices = new ArrayList<>();
    public final MutableLiveData<List<LocalServiceHandler>> mldLocalServices = new MutableLiveData<>();

    private final MutableLiveData<Boolean> isWifiConnected = new MutableLiveData<>();
    public final MutableLiveData<List<LocalInetInfo>> localInetInfo = new MutableLiveData<>();
    public final MutableLiveData<List<BeaconInfo>> beaconInfo = new MutableLiveData<>();
    public final MutableLiveData<Map<String, Router.NodeInfoItem>> mldNodeInfo = new MutableLiveData<>();

    public final MutableLiveData<List<String>> activeConnections = new MutableLiveData<>();
    public final MutableLiveData<Set<String>> storedConnections = new MutableLiveData<>();
    public final MutableLiveData<Integer> numActiveConnections = new MutableLiveData<>();
    public final MutableLiveData<List<BluetoothDevice>> bluetoothDevices = new MutableLiveData<>();

    public final TreeMap<String, BleUartChannel> addressToBleChannelMap = new TreeMap<>();
    public final MutableLiveData<TreeMap<String, BleUartChannel>> mldDddressToBleChannelMap = new MutableLiveData<>();

    public final MutableLiveData<String> bluetoothDiscoveryStatus = new MutableLiveData<>();
    public final MutableLiveData<Boolean> bluetoothDiscoveryIsActive = new MutableLiveData<>();

    public final MutableLiveData<String> qrDialogLabel = new MutableLiveData<>();
    public final MutableLiveData<String> qrScanResult = new MutableLiveData<>();


    HashMap<String, UDPListener> bcastListenMap = new HashMap<>();
    HashMap<String, ServerSocketChannel> serverSocketChannelMap = new HashMap<>();

    TreeMap<String, IChannel> connectedChannels = new TreeMap<>();

    Set<String> services;
    TreeMap<String, Set<String>> actions = new TreeMap<>();
    TreeSet<String> connections = new TreeSet();

    // url to channel; not great as it assumes at most one channel to a single url
    HashMap<String, IChannel> channelMap = new HashMap<>();
    public Router alnRouter;

    BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

    public static App getInstance() {
        return instance;
    }

    public void send(Packet packet) {
        alnRouter.send(packet);
    }

    private void updateWifi() {
        localInetInfo.setValue(NetUtil.getLocalInetInfo());
    }

    private void updateActiveChannels() {
        ArrayList<String> keys = new ArrayList<>();
        for (String k : connectedChannels.keySet()) {
            keys.add(k);
        }
        activeConnections.postValue(keys);
    }

    @Override
    public void onCreate() {
        instance = this;
        super.onCreate();

        loadServices();
        loadSavedConnections();
        numActiveConnections.setValue(0);
        bluetoothDiscoveryIsActive.setValue(false);
        bluetoothDevices.setValue(new ArrayList<>());

        // use a consistent UUID for this node; create one if this is the first run
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        String localAddress = prefs.getString("__localAddress", "");
        if (localAddress.length() == 0) {
            String addr = UUID.randomUUID().toString();
            prefs.edit().putString("__localAddress", addr).apply();
            localAddress = addr;
        }
        alnRouter = new Router(localAddress);

        updateWifi();

        // subscribe to wifi status updates
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
        registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final String action = intent.getAction();
                if (action.equals(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION)) {
                    boolean isConnected = intent.getBooleanExtra(WifiManager.EXTRA_SUPPLICANT_CONNECTED, false);
                    isWifiConnected.setValue(isConnected);
                    if (isConnected) {
                        final Handler handler = new Handler(Looper.getMainLooper());
                        handler.postDelayed(() -> updateWifi(), 5000); // wait a long time before sync'ing UI
                    }
                }
            }
        }, intentFilter);

        // Default local service
        addLocalService("log");

        mldNodeInfo.setValue(alnRouter.availableServices());
        alnRouter.setOnStateChangedListener(new Router.OnStateChangedListener() {
            @Override
            public void onStateChanged() {
                mldNodeInfo.postValue(alnRouter.availableServices());

                // clear closed connections from activeConnections
                Set<String> removeSet = new HashSet<>();
                Set<IChannel> channelSet = alnRouter.getChannelSet();
                for (String key : connectedChannels.keySet()) {
                    if (!channelSet.contains(connectedChannels.get(key))) {
                        removeSet.add(key);
                    }
                }
                for (String key : removeSet) {
                    connectedChannels.remove(key);
                }
                updateActiveChannels();
            }
        });
    }

    public void addLocalService(String name) {
        LocalServiceHandler lsh = new LocalServiceHandler(name);
        lsh.setOnChangedHandler(() -> mldLocalServices.postValue(localServices));
        alnRouter.registerService(lsh.service, lsh);
        localServices.add(lsh);
        mldLocalServices.setValue(localServices);
        Map<String, Router.NodeInfoItem> as = alnRouter.availableServices();
        mldNodeInfo.setValue(as);
    }

    public void removeLocalService(String name) {
        alnRouter.unregisterService(name);
        for (int i = 0; i < localServices.size(); i++) {
            if (localServices.get(i).service.equals(name)) {
                localServices.remove(i--); // remove at i; inspect at i again
            }
        }
        mldLocalServices.setValue(localServices);
    }

    UDPListener makeUDPListener(InetAddress bcastAddress, short port) {
        UDPListener listener = new UDPListener(bcastAddress, port);
        listener.setMessageHandler(new UDPListener.MessageHandler() {
            @Override
            public void onMessage(byte[] message) {
                String uri = new String(message);
                BeaconInfo info = NetUtil.beaconInfoFromUri(uri);
                if (info == null) {
                    Log.d("ALNN", "failed to parse bcast msg:" + uri);
                    return;
                }
                List<BeaconInfo> infos = beaconInfo.getValue();
                if (infos == null) {
                    infos = new ArrayList<BeaconInfo>();
                }
                for (BeaconInfo _info : infos) {
                    if (info.equals(_info))
                        return;
                }
                infos.add(info);
                beaconInfo.postValue(infos);
                Log.d("ALNN", uri);
            }
        });
        return listener;
    }

    public void listenToUDP(InetAddress bcastAddress, short port, boolean listen) {
        synchronized (bcastListenMap) {
            String path = String.format("%s:%d", bcastAddress.toString(), port);
            UDPListener listener;
            if (bcastListenMap.containsKey(path)) {
                listener = bcastListenMap.get(path);
            } else {
                listener = makeUDPListener(bcastAddress, port);
                listener.start();
                bcastListenMap.put(path, listener);
                return;
            }
            if (listen && !listener.isRunning()) {
                Log.d("ALNN", "listening");
                listener = makeUDPListener(bcastAddress, port);
                listener.start();
                bcastListenMap.put(path, listener);
            } else if (!listen && listener.isRunning()) {
                listener.end();
            }
            localInetInfo.setValue(localInetInfo.getValue()); // trigger observers
        }
    }

    public boolean isListeningToUDP(InetAddress bcastAddress, short port) {
        String path = String.format("%s:%d", bcastAddress.toString(), port);
        synchronized (bcastListenMap) {
            if (!bcastListenMap.containsKey(path)) {
                return false;
            }
            return bcastListenMap.get(path).isRunning();
        }
    }

    public void listenToTCP(InetAddress listenAddress, short port, boolean listen) {
        Log.d("ALNN", "listenToTCP");
        String key = String.format("%s:%d", listenAddress.toString(), port);

        if (listen == false) {
            // stop the server thread
            if (serverSocketChannelMap.containsKey(key)) {
                try {
                    serverSocketChannelMap.get(key).close();
                } catch (IOException e) {
                    Log.e("ALNN", "error stopping thread:" + e.getLocalizedMessage());
                    // throw new RuntimeException(e);
                }
                serverSocketChannelMap.remove(key);
            }
            return;
        }

        ServerSocketChannel ssc;
        try {
            ssc = ServerSocketChannel.open();
            ssc.socket().bind(new InetSocketAddress(listenAddress, port));
        } catch (IOException e) {
            Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_SHORT);
            return;
        }
        serverSocketChannelMap.put(key, ssc);

        new Thread(() -> {
            try {
                Log.d("ALNN", String.format("listening to %s:%d", listenAddress, port));
                while(true) {
                    SocketChannel clientSocket = ssc.accept();
                    String remoteHost = clientSocket.socket().getRemoteSocketAddress().toString();
                    int localPort = clientSocket.socket().getLocalPort();
                    Log.d("ALNN", String.format("accepting connection from %s:%d", remoteHost, localPort));
                    IChannel ch = new TcpChannel(clientSocket);
                    getInstance().alnRouter.addChannel(ch);
                    getInstance().channelMap.put(remoteHost, ch);
                    connectedChannels.put(remoteHost, ch);
                    updateActiveChannels();
                }
            } catch (IOException e) { }
        }).start();
    }

    public boolean isListeningToTCP(InetAddress listenAddress, short port) {
        String key = String.format("%s:%d", listenAddress.toString(), port);
        return serverSocketChannelMap.containsKey(key);
    }

    HashMap<String, Thread> broadcastUDPThreadMap = new HashMap<>();
    public void broadcastUDP(InetAddress bcastAddress, short port, String message, boolean enable) {
        String key = String.format("%s:%d", bcastAddress, port);
        Log.d("ALNN", "broadcastUDP - TODO");
        if (!enable) {
            if (broadcastUDPThreadMap.containsKey(key)) {
                try {
                    broadcastUDPThreadMap.get(key).stop();
                } catch (Exception e) { /* don't care */ }
                broadcastUDPThreadMap.remove(key);
            }
            return;
        }
        if (broadcastUDPThreadMap.containsKey(key))
            return; // do nothing if it's already running
        Thread newAdvertiserThread = new Thread(() -> {
            while(true) {
                try {
                    Thread.sleep(2000);
                    DatagramSocket socket = new DatagramSocket();
                    socket.setBroadcast(true);
                    byte[] sendData = message.getBytes();
                    DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, bcastAddress, port);
                    socket.send(sendPacket);
                } catch (IOException e) {
                    Log.e("ALNN", "IOException: " + e.getMessage());
                    break;
                } catch (Exception e) {
                    Log.e("ALNN", "Exception: " + e.getMessage());
                    break;
                }
            }
        });
        newAdvertiserThread.start();
        broadcastUDPThreadMap.put(key, newAdvertiserThread);
    }

    public boolean isBroadcastingUDP(InetAddress bcastAddress, short port) {
        String key = String.format("%s:%d", bcastAddress, port);
        return broadcastUDPThreadMap.containsKey(key);
    }



    BleUartSerial bleUartSerial;
    @SuppressLint("MissingPermission")
    public String connectTo(BluetoothDevice device, boolean enable) {
        String path = String.format("%s:%s", device.getAddress(), device.getName());
        if (enable) {
            bleUartSerial = new BleUartSerial(device);
            bleUartSerial.setOnConnectHandler(isConnected -> {
                Log.i("ALNN", "onConnect:" + isConnected);
                IChannel channel = new BleUartChannel(bleUartSerial);
                alnRouter.addChannel(channel);
                channelMap.put(path, channel);
                bluetoothDevices.postValue(bluetoothDevices.getValue());
            });
            bleUartSerial.connect();
        } else if (channelMap.containsKey(path)) {
            channelMap.get(path).close();
            channelMap.remove(path);
        }
        numActiveConnections.postValue(channelMap.size());
        bluetoothDevices.postValue(bluetoothDevices.getValue()); // trigger redraw
        return null;
    }
    @SuppressLint("MissingPermission")
    public boolean isConnected(BluetoothDevice device) {
        String path = String.format("%s:%s", device.getAddress(), device.getName());
        synchronized (channelMap) {
            return channelMap.containsKey(path);
        }
    }

    public String connectTo(String protocol, String host, short port, String node, boolean enable) {
        synchronized (channelMap) {
            String path = String.format("%s:%d", host, port);
            if (enable && !channelMap.containsKey(path)) {
                Packet p;
                IChannel channel;
                switch (protocol) {
                    case "tcp+aln":
                        channel = new TcpChannel(host, port);
                        break;

                    case "tcp+maln":
                        channel = new TcpChannel(host, port);
                        p = new Packet();
                        p.DestAddress = node;
                        channel.send(p);
                        break;

                    case "tls+aln":
                        channel = new TlsChannel(host, port);
                        break;

                    case "tls+maln":
                        channel = new TlsChannel(host, port);
                        p = new Packet();
                        p.DestAddress = node;
                        channel.send(p);
                        break;

                    default:
                        return "protocol not supported";
                }
                alnRouter.addChannel(channel);
                channelMap.put(path, channel);
            } else if (!enable && channelMap.containsKey(path)) {
                channelMap.get(path).close();
                channelMap.remove(path);
            }
            numActiveConnections.postValue(channelMap.size());
        }
        localInetInfo.setValue(localInetInfo.getValue()); // trigger observers
        return null;
    }

    public boolean isConnected(String protocol, String host, short port) {
        String path = String.format("%s:%d", host, port);
        synchronized (channelMap) {
            return channelMap.containsKey(path);
        }
    }

    public void disconnectActiveConnection(String key) {
        IChannel ch = connectedChannels.get(key);
        if (ch != null) {
            connectedChannels.remove(key);
            ch.close();
        }
        updateActiveChannels();
    }

    public void saveActionItem(String service, String title, String content) {
        if (!actions.containsKey(service)) {
            actions.put(service, new TreeSet<>());
        }

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        Set<String> existingCheck = prefs.getStringSet(service, new TreeSet<>());
        Log.d("ALNN", String.format("%d exist", existingCheck.size()));

        actions.get(service).add(String.format("%s\t%s", title, content));

        SharedPreferences.Editor editor = prefs.edit();
        editor.putStringSet(service, actions.get(service));
        if (!services.contains(service)) {
            services.add(service);
            editor.putStringSet("__services", services);
        }
        editor.apply();
    }

    public void replaceActionItem(String service, String prevTitle, String prevContent, String title, String content) {
        if (!actions.containsKey(service)) {
            actions.put(service, new TreeSet<>());
        }

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        Set<String> existingCheck = prefs.getStringSet(service, new TreeSet<>());
        Log.d("ALNN", String.format("%d exist", existingCheck.size()));

        actions.get(service).remove(String.format("%s\t%s", prevTitle, prevContent));
        actions.get(service).add(String.format("%s\t%s", title, content));

        SharedPreferences.Editor editor = prefs.edit();
        editor.putStringSet(service, actions.get(service));
        if (!services.contains(service)) {
            services.add(service);
            editor.putStringSet("__services", services);
        }
        editor.apply();
    }

    private void loadServices() {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        services = prefs.getStringSet("__services", new TreeSet<>());

        for (String service : services) {
            Set<String> records = prefs.getStringSet(service, new TreeSet<>());
            TreeSet<String> copy = new TreeSet<>();
            for (String record : records) copy.add(record);
            actions.put(service, copy);
        }
    }

    public Set<String> getActionsForService(String service) {
        return actions.get(service);
    }

    private void loadSavedConnections() {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        Set<String> storedConns = prefs.getStringSet("__connections", new TreeSet<>());

        for (String connection : storedConns) {
            connections.add(connection);
        }
        storedConnections.postValue(connections);
    }

    public void saveConnection(String title, String url) {
        if ((title.length() + url.length()) == 0)
            return;

        connections.add(String.format("%s\t%s", title, url));

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        prefs.edit().putStringSet("__connections", connections).apply();

        storedConnections.postValue(connections);
    }

    public void forgetConnection(String content) {
        TreeSet<String> newConnections = new TreeSet();
        for (String connection : connections) {
            if (!connection.equals(content)) {
                newConnections.add(connection);
            }
        }
        connections = newConnections;
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        prefs.edit().putStringSet("__connections", connections).apply();

        storedConnections.postValue(connections);
    }

    public short getUdpAdvertPortForInterface(String iface) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        int port = prefs.getInt("__udp_advert_port_for_"+iface, 8082);
        return (short)port;
    }

    public void setUdpAdvertPortForInterface(String iface, short port) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        prefs.edit().putInt("__udp_advert_port_for_"+iface, port).apply();
        localInetInfo.setValue(localInetInfo.getValue());
    }

    public short getTcpHostPortForInterface(String iface) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        int port = prefs.getInt("__tcp_host_port_for_"+iface, 8081);
        return (short)port;
    }

    public void setTcpHostPortForInterface(String iface, short port) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        prefs.edit().putInt("__tcp_host_port_for_"+iface, port).apply();
        localInetInfo.setValue(localInetInfo.getValue());
    }


    public boolean getActAsHostForInterface(String iface) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        return prefs.getBoolean("__act_as_host_for_"+iface, false);
    }

    public void setActAsHostForInterface(String iface, boolean actAsHost) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        prefs.edit().putBoolean("__act_as_host_for_"+iface, actAsHost).apply();
        localInetInfo.setValue(localInetInfo.getValue());
    }

    public boolean addBLEDevice(BluetoothDevice bleDevice) {
        List<BluetoothDevice> devices = bluetoothDevices.getValue();
        for(BluetoothDevice bd : devices) {
            if (bd.getAddress().equals(bleDevice.getAddress())) {
                return false; // already have this device
            }
        }
        devices.add(bleDevice);
        bluetoothDevices.postValue(devices);

        String msg = String.format("%d devices discovered", devices.size());
        bluetoothDiscoveryStatus.postValue(msg);
        return true;
    }

    public void startBLEScan() {
        bluetoothDiscoveryIsActive.setValue(true);
    }

    public void stopBLEScan() {
        bluetoothDiscoveryIsActive.setValue(false);
    }
//
//    public void toggleBluetoothDiscovery() {
//        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
//            return;
//        }
//        if (bluetoothDiscoveryIsActive.getValue().booleanValue()) {
//            bluetoothAdapter.cancelDiscovery();
//            bluetoothDiscoveryStatus.setValue("discovery canceled");
//            bluetoothDiscoveryIsActive.setValue(false);
//            return;
//        }
//
//        bluetoothDiscoveryStatus.setValue("initializing discovery...");
//        bluetoothDevices.setValue(new ArrayList<>());
//
//        IntentFilter filter = new IntentFilter();
//        filter.addAction(BluetoothDevice.ACTION_FOUND);
//        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
//        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
//        registerReceiver(new BroadcastReceiver() {
//
//            @SuppressLint("MissingPermission")
//            @Override
//            public void onReceive(Context context, Intent intent) {
//                final String action = intent.getAction();
//                List<BluetoothDevice> devices = bluetoothDevices.getValue();
//                if (BluetoothDevice.ACTION_FOUND.equals(action)) {
//                    BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
//                    if (!devices.contains(device)) {
//                        devices.add(device);
//                    }
//                } else if (BluetoothAdapter.ACTION_DISCOVERY_STARTED.equals(intent.getAction())) {
//                    bluetoothDiscoveryStatus.setValue("discovery started");
//                } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(intent.getAction())) {
//                    bluetoothDiscoveryStatus.setValue("discovery finished");
//                    bluetoothDiscoveryIsActive.setValue(false);
//                }
//                String msg = String.format("%d bluetooth devices found", devices.size());
//                bluetoothDiscoveryStatus.setValue(msg);
//                bluetoothDevices.postValue(devices);
//            }
//        }, filter);
//
//        bluetoothAdapter.startDiscovery();
//        bluetoothDiscoveryIsActive.setValue(true);
//    }

    public short getNetListenPortForInterface(String iface) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getInstance());
        int port = prefs.getInt("__port_for_"+iface, 8082);
        return (short)port;
    }

}
