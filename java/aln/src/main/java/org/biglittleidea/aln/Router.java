package org.biglittleidea.aln;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.concurrent.ThreadLocalRandom;


public class Router {
    /*
    An ALN is a set of routers connected by channels.
    Nodes may be local or remote addresses on the network.
    */
    public Router(String address) {
        this.address = address;
    }

    public class ServiceNodeInfo {
        public String service; // name of the service
        public String address; // host of the service
        public String nextHop; // next routing node for address
        public Short load; // remote service load factor
        public String err; // parser error
    }

    // contextHandlerMap stores query response handers by contextID
    HashMap<Short,IPacketHandler> contextHandlerMap = new HashMap<Short,IPacketHandler>();

    // serviceHandlerMap maps local endpoint nodes to packet handlers
    HashMap<String,IPacketHandler> serviceHandlerMap = new HashMap<String,IPacketHandler>();

    public class RemoteNodeInfo {
        public String address; // target addres to communicate with
        public String nextHop; // next routing node for address
        public IChannel channel;     // specific channel that is hosting next hop
        public short cost;      // cost of using this route, generally a hop count
        public Date lastSeen;   // routes should decay with a few missed updates

        public String err; // parser err msg
    }

    // map[address]RemoteNode
    HashMap<String, RemoteNodeInfo> remoteNodeMap = new HashMap<String, RemoteNodeInfo>();

    class NodeLoad {
        short Load;
        Date lastSeen;
    }

    // map[service][address]NodeLoad
    HashMap<String, HashMap<String, NodeLoad>> serviceLoadMap = 
        new HashMap<String, HashMap<String, NodeLoad>>();

    // Router manages a set of channels and packet handlers
    protected String address = "";
    ArrayList<IChannel> channels = new ArrayList<>();
    HashMap<String, ArrayList<Packet>> serviceQueue =
        new HashMap<String, ArrayList<Packet>>(); // packets waiting for services during warmup
    
    public String selectService(String service) {
        if (serviceHandlerMap.containsKey(service))
            return this.address;

        short minLoad = 0;
        String remoteAddress = "";
        if (serviceLoadMap.containsKey(service)) {
            HashMap<String, NodeLoad> addressToLoadMap = serviceLoadMap.get(service);
            for (String address : addressToLoadMap.keySet()) {
                NodeLoad nodeLoad = addressToLoadMap.get(address);
                if (minLoad == 0 || minLoad > nodeLoad.Load ) {
                    remoteAddress = address;
                    minLoad = nodeLoad.Load;
                }
            }
        }
        return remoteAddress;
    }

    // TODO normal java behavior is to throw exceptions, not return error messages
    public String send(Packet p) {
        synchronized(this) {
            if (p.SourceAddress == null || p.SourceAddress.length() == 0) {
                p.SourceAddress = this.address;
            }
            if ((p.DestAddress == null || p.DestAddress.length() == 0) &&
                (p.Service != null && p.Service.length() > 0) ) {
                p.DestAddress = selectService(p.Service);
                if (p.DestAddress.length() == 0) {
                    ArrayList<Packet> packets;
                    if (!serviceQueue.containsKey(p.Service)) {
                        packets = new ArrayList<>();
                        serviceQueue.put(p.Service, packets);
                    } else {
                        packets = serviceQueue.get(p.Service);
                    }
                    packets.add(p);
                    System.out.println("service unavailable, packet queued");
                }
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
        synchronized(this) {
            ctx = (short)ThreadLocalRandom.current().nextInt(1, 2<<16);
            this.contextHandlerMap.put(ctx, handler);
        }
        return ctx;
    }

    public void releaseContext(short ctx) {
        synchronized(this) {
            this.contextHandlerMap.remove(ctx);
        }
    }

    protected Packet composeNetRouteShare(String address, short cost) {
        Packet p = new Packet();
        p.Net = Packet.NetState.ROUTE;
        p.SourceAddress = this.address;
        p.Data = new byte[address.length()+3];
        ByteBuffer buffer = ByteBuffer.allocate(Frame.BufferSize);
        buffer.put((byte)address.length());
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
            info.err = String.format("parseNetworkRouteSharePacket: packet.data.length: %d; exp: %d", data.length, addrSize + 3);
            return info;
          }

          int offset = 1;
          info.address = new String(Arrays.copyOfRange(data, offset, offset+addrSize));
          offset += addrSize;
          info.cost = Packet.readUINT16(data, offset);
          offset += 2;
          info.nextHop = packet.SourceAddress;

          return info;
    }

    protected Packet composeNetServiceShare(String address, String service, short load) {
        Packet p = new Packet();
        p.Net = Packet.NetState.SERVICE;
        p.SourceAddress = this.address;
        p.Data = new byte[this.address.length() + service.length() + 4];
        ByteBuffer buffer = ByteBuffer.allocate(Frame.BufferSize);
        buffer.put((byte)address.length());
        buffer.put(address.getBytes());
        buffer.put((byte)service.length());
        buffer.put(service.getBytes());
        buffer.put(Packet.writeUINT16(load));
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
          info.address = new String(Arrays.copyOfRange(data, offset, offset+addrSize));
          offset += addrSize;

          byte srvSize = data[offset];
          offset += 1;
          info.service = new String(Arrays.copyOfRange(data, offset, offset+srvSize));
          offset += srvSize;

          info.load = Packet.readUINT16(data, offset);
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
            System.out.printf("router '%s' recv'd ROUTE update\n", address);
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
                    for(IChannel ch : channels) {
                        if (ch != localInfo.channel) {
                            ch.send(packet);
                        }
                    }
                }
            } else {
                // add or update a route
                RemoteNodeInfo localInfo;
                synchronized(this) {
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
                    for(IChannel ch : channels) {
                        if (ch != channel) {
                            ch.send(p);
                        }
                    }
                }
            }
            
            break;

        case Packet.NetState.SERVICE:
        System.out.printf("router '%s' recv'd SERVICE update\n", address);
            ServiceNodeInfo serviceInfo = parseNetServiceShare(packet);
            if (serviceInfo.err != null) {
                System.out.printf("error parsing net service: %s\n", serviceInfo.err);
                return;
            }
            synchronized(this) {
                NodeLoad nodeLoad = new NodeLoad();
                nodeLoad.Load = serviceInfo.load;
                nodeLoad.lastSeen = new Date();
                HashMap<String, NodeLoad> loadMap;
                if (!serviceLoadMap.containsKey(serviceInfo.service)) {
                    loadMap = new HashMap<>();
                    serviceLoadMap.put(serviceInfo.service, loadMap);
                } else {
                    loadMap = serviceLoadMap.get(serviceInfo.service);       
                    if (loadMap.containsKey(serviceInfo.address) &&
                        loadMap.get(serviceInfo.address).Load == nodeLoad.Load) {
                        return; // drop redunant packet to avoid propagation loops
                    }
                }

                loadMap.put(serviceInfo.address, nodeLoad);

                if (serviceQueue.containsKey(serviceInfo.service)) {
                    ArrayList<Packet> sq = serviceQueue.get(serviceInfo.service);
                    System.out.printf("sending %d packet(s) to '%s'\n", sq.size(), serviceInfo.address);
                    if (remoteNodeMap.containsKey(serviceInfo.address)) {
                        RemoteNodeInfo routeInfo = remoteNodeMap.get(serviceInfo.address);
                        for (Packet p : sq) {
                            p.DestAddress = serviceInfo.address;
                            p.NextAddress = routeInfo.nextHop;
                            routeInfo.channel.send(p);
                        }
                        serviceQueue.remove(serviceInfo.service);
                    } else {
                        System.out.printf("no route to discovered service %s\n",serviceInfo.service);
                    }
                }
            }
            // forward the service load
            for (IChannel ch: channels) {
                if (ch != channel) {
                    ch.send(packet);
                }
            }
            break;

        case Packet.NetState.QUERY:
        System.out.printf("router '%s' recv'd QUERY\n", address);
            for (Packet p : exportRouteTable()) channel.send(p);
            for (Packet p : exportServiceTable()) channel.send(p);
            break;
        }
    }

    public void addChannel(IChannel channel) {
        synchronized(this) {
            this.channels.add(channel);
        }

        channel.receive(new IPacketHandler(){
            @Override
            public void onPacket(Packet packet) {
                if (packet.Net != 0) {
                    handleNetState(channel, packet);
                } else {
                    send(packet);
                }
            }
        }, new IChannelCloseHandler(){
            @Override
            public void onChannelClosed(IChannel ch) {
                removeChannel(ch);     
            }
        });
        System.out.printf("router '%s' sending QUERY\n", address);
        
        channel.send(composeNetQuery()); // immediately query the new connection
    }

    public void removeChannel(IChannel ch) {
        System.out.println("router:RemoveChannel");
        synchronized(this) {
            channels.remove(ch);
            // bcast the loss of routes through the channel
            for (String address : remoteNodeMap.keySet()) {
                System.out.printf("router:RemoveChannel address '%s'\n", address);
                RemoteNodeInfo nodeInfo = remoteNodeMap.get(address);
                if (nodeInfo.channel == ch) {
                    removeAddress(address);
                    for (IChannel c : channels) {
                        c.send(composeNetRouteShare(address, (short)0));
                    }
                }
            }
        }
    }

    protected void removeAddress(String address) {
        synchronized(this) {
            remoteNodeMap.remove(address);
            for (HashMap<String, NodeLoad> nlm : serviceLoadMap.values()) {
                nlm.remove(address);
            }
        }
    }

    public void registerService(String service, IPacketHandler handler) {
        synchronized(this) {
            serviceHandlerMap.put(service, handler);
        }        
    }

    public void unregisterService(String service) {
        synchronized(this) {
            serviceHandlerMap.remove(service);
        }
    }

    public Packet[] exportRouteTable() {
        Packet[] routes = new Packet[remoteNodeMap.size()+1];
        int i = 0;
        routes[i++] = composeNetRouteShare(address, (short)1);
        for (String address : remoteNodeMap.keySet()) {
            RemoteNodeInfo routeInfo =  remoteNodeMap.get(address);
            routes[i++] = composeNetRouteShare(address, (short)(routeInfo.cost+1));
        }
        return routes;
    }

    public Packet[] exportServiceTable() {
        int sz = serviceHandlerMap.size();
        for (HashMap<String,NodeLoad> loadMap : serviceLoadMap.values()) {
            sz += loadMap.size();
        }
        Packet[] services = new Packet[sz];
        int i = 0;
        for (String service : serviceHandlerMap.keySet()) {
            services[i++] = composeNetServiceShare(this.address, service, (short)0);
        }
        for (String service : serviceLoadMap.keySet()) {
            HashMap<String,NodeLoad> loadMap = serviceLoadMap.get(service);
            for ( String address : loadMap.keySet()) {
                NodeLoad loadInfo = loadMap.get(address);
                services[i++] = composeNetServiceShare(address, service, loadInfo.Load);
            }
        }
        return services;
    }

    public void shareNetState() {
        Packet[] routes, services;
        synchronized(this){
            routes = exportRouteTable();
            services = exportServiceTable();
        }
        for (IChannel ch : channels) {
            for (Packet p : routes) ch.send(p);
            for (Packet p : services) ch.send(p);
        }    
    }

    public int numChannels() {
        synchronized(this) {
            return channels.size();
        }
    }

}
