package biglittleidea.alnn.ui.wifi;

public class StoredConnectionItem {
    public String title;
    public String protocol;
    public String host;
    public short port;
    public String node;
    public String content;

    public StoredConnectionItem(String content) {
        String title, protocol, address, host, node = "";
        short port = 8181;
        String[] parts = content.split("\t");
        switch (parts.length) {
            case 2:
                title = parts[0];
                parts = parts[1].split("://");
                if (parts.length == 2) {
                    protocol = parts[0];
                    address = parts[1];
                } else{
                    protocol = "unknown";
                    address = parts[0];
                }
                break;
            case 3:
                title = parts[0];
                protocol = parts[1];
                address = parts[2];
                break;
            default:
                protocol = "";
                address = parts[0];
                title = "untitled";
        }
        parts = address.split("/");
        address = parts[0];
        if (parts.length == 2) {
            node = parts[1];
        }
        parts = address.split(":");
        host = parts[0];
        if (parts.length == 2) {
            port = Short.parseShort(parts[1]);
        }
        this.title = title;
        this.protocol = protocol;
        this.host = host;
        this.port = port;
        this.node = node;
        this.content = content;
    }

    public StoredConnectionItem(String title, String protocol, String host, short port, String node) {
        this.title = title;
        this.protocol = protocol;
        this.host = host;
        this.port = port;
        this.node = node;
    }
}
