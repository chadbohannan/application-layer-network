package biglittleidea.alnn;

public class BeaconInfo {
    public String protocol;
    public String host;
    public short port;
    public String path;

    public BeaconInfo(String protocol, String host, short port, String path) {
        this.protocol = protocol;
        this.host = host;
        this.port = port;
        this.path = path;
    }

    public boolean equals(BeaconInfo info) {
        return info.protocol.equals(protocol) &&
            info.host.equals(host) &&
            info.port == port &&
            info.path.equals(path);
    }
}
