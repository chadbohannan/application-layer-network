package org.biglittleidea.aln;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.concurrent.ThreadLocalRandom;

public class Router {
    public Router(String address) {
        this.address = address;
    }

    public class ServiceNodeInfo {
        public String service; // name of the service
        public String address; // host of the service
        public String nextHop; // next routing node for address
        public Short capacity; // remote service capacity; zero means unavailable
        public String err; // parser error
    }

    // contextHandlerMap stores query response handers by contextID
    HashMap<Short, IPacketHandler> contextHandlerMap = new HashMap<Short, IPacketHandler>();

    // serviceHandlerMap maps local endpoint nodes to packet handlers
    HashMap<String, IPacketHandler> serviceHandlerMap = new HashMap<String, IPacketHandler>();

    public class RemoteNodeInfo {
        public String address; // target addres to communicate with
        public String nextHop; // next routing node for address
        public IChannel channel; // specific channel that is hosting next hop
        public short cost; // cost of using this route, generally a hop count
        public Date lastSeen; // routes should decay with a few missed updates

        public String err; // parser err msg
    }

    // map[address]RemoteNode
    HashMap<String, RemoteNodeInfo> remoteNodeMap = new HashMap<String, RemoteNodeInfo>();

    class NodeCapacity {
        short capacity;
        Date lastSeen;
    }

    // map[service][address]NodeCapacity
    HashMap<String, HashMap<String, NodeCapacity>> serviceCapacityMap = new HashMap<String, HashMap<String, NodeCapacity>>();

    // Router manages a set of channels and packet handlers
    protected String address = "";
    ArrayList<IChannel> channels = new ArrayList<>();

    ArrayList<String> serviceAddresses(String service) {
        ArrayList<String> addressList = new ArrayList<>();
        synchronized(this) {
            if (serviceHandlerMap.containsKey(service))
                addressList.add(this.address);

            if (serviceCapacityMap.containsKey(service)) {
                HashMap<String, NodeCapacity> addressToCapacityMap = serviceCapacityMap.get(service);
                for (String address : addressToCapacityMap.keySet()) {
                    addressList.add(address);
                }
            }
        }
        return addressList;
    }

    public String send(Packet p) {
        if (p.SourceAddress == null || p.SourceAddress.length() == 0) {
            p.SourceAddress = this.address;
        }
        if ((p.DestAddress == null || p.DestAddress.length() == 0) &&
                (p.Service != null && p.Service.length() > 0)) {
            ArrayList<String> addresses = this.serviceAddresses(p.Service);
            if (addresses.size() > 0) {
                for (int i = 1; i < addresses.size(); i++) {
                    Packet pc = p.copy();
                    pc.DestAddress = addresses.get(i);
                    this.send(pc);
                }
                p.DestAddress = addresses.get(0);
            } else {
                return String.format("no services of type %s yet discovered on the network", p.Service);
            }
        }

        if (p.DestAddress.equals(this.address)) {
            IPacketHandler handler;
            if (this.serviceHandlerMap.containsKey(p.Service)) {
                handler = this.serviceHandlerMap.get(p.Service);
                handler.onPacket(p);
            } else if (this.contextHandlerMap.containsKey(p.ContextID)) {
                handler = this.contextHandlerMap.get(p.ContextID);
                handler.onPacket(p);
            } else {
                return System.out.printf("service '%s' not registered\n", p.Service).toString();
            }
        } else if (p.NextAddress == null || p.NextAddress.equals(this.address) || p.NextAddress.length() == 0) {
            if (this.remoteNodeMap.containsKey(p.DestAddress)) {
                RemoteNodeInfo rni = this.remoteNodeMap.get(p.DestAddress);
                p.NextAddress = rni.nextHop;
                rni.channel.send(p);
            }
        } else {
            return "packet is unroutable; no action taken";
        }
        return null;
    }

    public short registerContextHandler(IPacketHandler handler) {
        short ctx;
        synchronized (this) {
            ctx = (short) ThreadLocalRandom.current().nextInt(1, 2 << 16);
            while (contextHandlerMap.containsKey(ctx))
                ctx = (short) ThreadLocalRandom.current().nextInt(1, 2 << 16);;
            contextHandlerMap.put(ctx, handler);
        }
        return ctx;
    }

    public void releaseContext(short ctx) {
        synchronized (this) {
            this.contextHandlerMap.remove(ctx);
        }
    }

    protected Packet composeNetRouteShare(String address, short cost) {
        Packet p = new Packet();
        p.Net = Packet.NetState.ROUTE;
        p.SourceAddress = this.address;
        p.Data = new byte[address.length() + 3];
        ByteBuffer buffer = ByteBuffer.allocate(Frame.BufferSize);
        buffer.put((byte) address.length());
        buffer.put(address.getBytes());
        buffer.put(Packet.writeUINT16(cost));
        buffer.rewind();
        buffer.get(p.Data);
        return p;
    }

    protected RemoteNodeInfo parseNetRouteShare(Packet packet) {
        RemoteNodeInfo info = new RemoteNodeInfo();
        if (packet.Net != Packet.NetState.ROUTE) {
            info.err = "parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE";
            return info;
        }
        if (packet.Data == null || packet.Data.length == 0) {
            info.err = "parseNetworkRouteSharePacket: packet.Data is empty";
            return info;
        }

        byte[] data = packet.Data;
        byte addrSize = data[0];

        if (data.length != addrSize + 3) {
            info.err = String.format("parseNetworkRouteSharePacket: len: %d; exp: %d", data.length, addrSize + 3);
            return info;
        }

        int offset = 1;
        info.address = new String(Arrays.copyOfRange(data, offset, offset + addrSize));
        offset += addrSize;
        info.cost = Packet.readUINT16(data, offset);
        info.nextHop = packet.SourceAddress;
        return info;
    }

    protected Packet composeNetServiceShare(String address, String service, short capacity) {
        Packet p = new Packet();
        p.Net = Packet.NetState.SERVICE;
        p.SourceAddress = this.address;
        p.Data = new byte[this.address.length() + service.length() + 4];
        ByteBuffer buffer = ByteBuffer.allocate(Frame.BufferSize);
        buffer.put((byte) address.length());
        buffer.put(address.getBytes());
        buffer.put((byte) service.length());
        buffer.put(service.getBytes());
        buffer.put(Packet.writeUINT16(capacity));
        buffer.rewind();
        buffer.get(p.Data, 0, p.Data.length);
        return p;
    }

    protected ServiceNodeInfo parseNetServiceShare(Packet packet) {
        ServiceNodeInfo info = new ServiceNodeInfo();
        if (packet.Net != Packet.NetState.SERVICE) {
            info.err = "parseNetworkRouteSharePacket: packet.NetState != NET_SERVICE";
            return info;
        }
        if (packet.Data == null || packet.Data.length == 0) {
            info.err = "parseNetworkRouteSharePacket: packet.Data is empty";
            return info;
        }

        byte[] data = packet.Data;
        byte addrSize = data[0];
        int offset = 1;
        info.address = new String(Arrays.copyOfRange(data, offset, offset + addrSize));
        offset += addrSize;

        byte srvSize = data[offset];
        offset += 1;
        info.service = new String(Arrays.copyOfRange(data, offset, offset + srvSize));
        offset += srvSize;

        info.capacity = Packet.readUINT16(data, offset);
        return info;
    }

    protected Packet composeNetQuery() {
        Packet p = new Packet();
        p.Net = Packet.NetState.QUERY;
        return p;
    }

    protected void handleNetState(IChannel channel, Packet packet) {
        switch (packet.Net) {
            case Packet.NetState.ROUTE:
                // System.out.printf("router '%s' recv'd ROUTE update\n", address);
                // neighbor is sharing it's routing table
                RemoteNodeInfo info = parseNetRouteShare(packet);
                if (info.err != null) {
                    System.out.println(info.err);
                    return;
                }

                // msg := "NET_ROUTE [%s] remoteAddress:%s, nextHop:%s, cost:%d\n"
                // log.Printf(msg, r.address, remoteAddress, nextHop, cost)
                if (info.cost == 0) { // zero cost routes are removed
                    if (info.address.equals(this.address)) {
                        // fight the lie
                        try {
                            Thread.sleep(500);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        this.shareNetState();
                        return;
                    } else if (this.remoteNodeMap.containsKey(info.address)) {
                        // remove the route

                        RemoteNodeInfo localInfo = remoteNodeMap.get(info.address);
                        removeAddress(info.address);
                        for (IChannel ch : channels) {
                            if (ch != localInfo.channel) {
                                ch.send(packet);
                            }
                        }
                    }
                } else {
                    // add or update a route
                    RemoteNodeInfo localInfo;
                    synchronized (this) {
                        if (remoteNodeMap.containsKey(info.address)) {
                            localInfo = remoteNodeMap.get(info.address);
                        } else {
                            localInfo = new RemoteNodeInfo();
                            localInfo.address = info.address;
                            remoteNodeMap.put(info.address, localInfo);
                        }
                    }
                    localInfo.lastSeen = new Date();
                    if (info.cost < localInfo.cost || localInfo.cost == 0) {
                        localInfo.channel = channel;
                        localInfo.cost = info.cost;
                        localInfo.nextHop = info.nextHop;
                        Packet p = composeNetRouteShare(info.address, ++info.cost);
                        for (IChannel ch : channels) {
                            if (ch != channel) {
                                ch.send(p);
                            }
                        }
                    }
                }

                break;

            case Packet.NetState.SERVICE:
                // System.out.printf("router '%s' recv'd SERVICE update\n", address);
                ServiceNodeInfo serviceInfo = parseNetServiceShare(packet);
                if (serviceInfo.err != null) {
                    System.out.printf("error parsing net service: %s\n", serviceInfo.err);
                    return;
                }
                if (serviceInfo.address == this.address) {
                    break;
                }
                synchronized (this) {
                    HashMap<String, NodeCapacity> capacityMap;
                    if (!serviceCapacityMap.containsKey(serviceInfo.service)) {
                        capacityMap = new HashMap<>();
                        serviceCapacityMap.put(serviceInfo.service, capacityMap);
                    } else if (serviceInfo.capacity == 0) {
                        capacityMap = serviceCapacityMap.get(serviceInfo.service);
                        if (capacityMap.containsKey(serviceInfo.address))
                            capacityMap.remove(serviceInfo.address);
                        else // we have no record of this service at this address
                            return; // drop packet to avoid propagation loops
                    } else {
                        capacityMap = serviceCapacityMap.get(serviceInfo.service);
                        if (capacityMap.containsKey(serviceInfo.address) &&
                                capacityMap.get(serviceInfo.address).capacity == serviceInfo.capacity) {
                            return; // drop packet to avoid propagation loops
                        }
                    }
                    NodeCapacity nodeCapacity = new NodeCapacity();
                    nodeCapacity.capacity = serviceInfo.capacity;
                    nodeCapacity.lastSeen = new Date();
                    capacityMap.put(serviceInfo.address, nodeCapacity);
                }
                // forward the service capacity
                for (IChannel ch : channels) {
                    if (ch != channel) {
                        ch.send(packet);
                    }
                }
                break;

            case Packet.NetState.QUERY:
                // System.out.printf("router '%s' recv'd QUERY\n", address);
                for (Packet p : exportRouteTable())
                    channel.send(p);
                for (Packet p : exportServiceTable())
                    channel.send(p);
                break;
        }
    }

    public void addChannel(IChannel channel) {
        synchronized (this) {
            this.channels.add(channel);
        }

        channel.receive(new IPacketHandler() {
            @Override
            public void onPacket(Packet packet) {
                if (packet.Net != 0) {
                    handleNetState(channel, packet);
                } else {
                    send(packet);
                }
            }
        }, new IChannelCloseHandler() {
            @Override
            public void onChannelClosed(IChannel ch) {
                removeChannel(ch);
            }
        });
        // System.out.printf("router '%s' sending QUERY\n", address);

        channel.send(composeNetQuery()); // immediately query the new connection
    }

    public void removeChannel(IChannel ch) {
        synchronized (this) {
            channels.remove(ch);
            // bcast the loss of routes through the channel
            for (String address : remoteNodeMap.keySet()) {
                // System.out.printf("router:RemoveChannel address '%s'\n", address);
                RemoteNodeInfo nodeInfo = remoteNodeMap.get(address);
                if (nodeInfo.channel == ch) {
                    removeAddress(address);
                    for (IChannel c : channels) {
                        c.send(composeNetRouteShare(address, (short) 0));
                    }
                }
            }
        }
    }

    protected void removeAddress(String address) {
        synchronized (this) {
            remoteNodeMap.remove(address);
            for (HashMap<String, NodeCapacity> nlm : serviceCapacityMap.values()) {
                nlm.remove(address);
            }
        }
    }

    public void registerService(String service, IPacketHandler handler) {
        synchronized (this) {
            serviceHandlerMap.put(service, handler);
        }
    }

    public void unregisterService(String service) {
        synchronized (this) {
            serviceHandlerMap.remove(service);
        }
    }

    public Packet[] exportRouteTable() {
        Packet[] routes = new Packet[remoteNodeMap.size() + 1];
        int i = 0;
        routes[i++] = composeNetRouteShare(address, (short) 1);
        for (String address : remoteNodeMap.keySet()) {
            RemoteNodeInfo routeInfo = remoteNodeMap.get(address);
            routes[i++] = composeNetRouteShare(address, (short) (routeInfo.cost + 1));
        }
        return routes;
    }

    public Packet[] exportServiceTable() {
        int sz = serviceHandlerMap.size();
        for (HashMap<String, NodeCapacity> capacityMap : serviceCapacityMap.values()) {
            sz += capacityMap.size();
        }
        Packet[] services = new Packet[sz];
        int i = 0;
        for (String service : serviceHandlerMap.keySet()) {
            short capacity = 1; // TODO make local capacity variable
            services[i++] = composeNetServiceShare(this.address, service, capacity);
        }
        for (String service : serviceCapacityMap.keySet()) {
            HashMap<String, NodeCapacity> capacityMap = serviceCapacityMap.get(service);
            for (String address : capacityMap.keySet()) {
                NodeCapacity capacityInfo = capacityMap.get(address);
                services[i++] = composeNetServiceShare(address, service, capacityInfo.capacity);
            }
        }
        return services;
    }

    public void shareNetState() {
        Packet[] routes, services;
        synchronized (this) {
            routes = exportRouteTable();
            services = exportServiceTable();
        }
        for (IChannel ch : channels) {
            for (Packet p : routes)
                ch.send(p);
            for (Packet p : services)
                ch.send(p);
        }
    }

    public int numChannels() {
        synchronized (this) {
            return channels.size();
        }
    }

}
