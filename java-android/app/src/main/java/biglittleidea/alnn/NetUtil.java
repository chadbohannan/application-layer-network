package biglittleidea.alnn;

import android.util.Log;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

public class NetUtil {

    public static List<LocalInetInfo> getLocalInetInfo() {
        List<LocalInetInfo> infos = new ArrayList<>();
        try {
            for (Enumeration<NetworkInterface> networkInterface =
                 NetworkInterface.getNetworkInterfaces(); networkInterface.hasMoreElements();) {
                NetworkInterface singleInterface = networkInterface.nextElement();
                String name = singleInterface.getDisplayName();
                for (Enumeration<InetAddress> IpAddresses = singleInterface.getInetAddresses();
                     IpAddresses.hasMoreElements();) {
                    LocalInetInfo info = new LocalInetInfo();
                    InetAddress inetAddress = IpAddresses.nextElement();
                    InetAddress bcastAddress = getBroadcastAddress(inetAddress);
                    if ((inetAddress instanceof Inet4Address) &&
                            !inetAddress.isLoopbackAddress() &&
                            bcastAddress != null) {
                        info.name = name;
                        info.inetAddress = inetAddress;
                        info.bcastAddress = bcastAddress;
                        infos.add(info);
                    }
                }
            }

        } catch (SocketException ex) {
            Log.e("ALNN", ex.toString());
        }
        return infos;
    }

    public static InetAddress getBroadcastAddress(InetAddress inetAddress) {
        try {
            NetworkInterface temp = NetworkInterface.getByInetAddress(inetAddress);
            List<InterfaceAddress> addresses = temp.getInterfaceAddresses();
            for (InterfaceAddress addr: addresses) {
                InetAddress bcast = addr.getBroadcast();
                if (bcast != null)
                    return bcast;
            }
        } catch (SocketException e) {
            e.printStackTrace();
            Log.d("ALNN", "getBroadcast" + e.getMessage());
        }
        return null;
    }

    public static BeaconInfo beaconInfoFromUri(String uri) {
        String[] parts = uri.split("://");
        short port = 0;
        if (parts.length != 2) {
            return new BeaconInfo("err", "invalid url", port, "");
        }
        String protocol = parts[0];
        parts = parts[1].split(":");
        String host = parts[0];
        if (parts.length != 2) {
            return new BeaconInfo(protocol, host, port, "");
        }
        parts = parts[1].split("/");
        port = Short.parseShort(parts[0]);
        String path = "";
        if (parts.length > 1)
            path = parts[1];
        return new BeaconInfo(protocol, host, port, path);
    }
}
