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

        if (p.DestAddress == this.address) {
            IPacketHandler handler;
            if (this.serviceHandlerMap.containsKey(p.Service)) {
                handler = this.serviceHandlerMap.get(p.Service);
                handler.onPacket(p);
            } else if (this.contextHandlerMap.containsKey(p.ContextID)) {
                handler = this.contextHandlerMap.get(p.ContextID);
                handler.onPacket(p);
            } else {
                return System.out.printf("service '%s' not registered", p.Service).toString();
            }
        } else if (p.NextAddress == this.address || p.NextAddress == null || p.NextAddress.length() == 0) {
            if (this.remoteNodeMap.containsKey(p.DestAddress)) {
                RemoteNodeInfo rni = this.remoteNodeMap.get(p.DestAddress);
                p.NextAddress = rni.nextHop;
                rni.channel.send(p);
            }else {
                return System.out.printf("packet queued for delayed send t %s", p.DestAddress).toString();
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

    protected Packet composeNetRouteShare(RemoteNodeInfo info) {
        Packet p = new Packet();
        p.Net = Packet.NetState.ROUTE;
        p.SourceAddress = this.address;
        p.Data = new byte[info.address.length()+3];
        ByteBuffer buffer = ByteBuffer.allocate(Frame.BufferSize);
        buffer.put((byte)info.address.length());
        buffer.put(info.address.getBytes());
        buffer.put(Packet.writeUINT16(info.cost));
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

    protected void handleNetState(Packet packet) {
        switch (packet.NetState) {
        case NET_ROUTE:
            // neighbor is sharing it's routing table
            RemoteNodeInfo info = parseNetRouteShare(packet);
            if (info.err != null) {
                System.out.println(info.err);
                return;
            }
            synchronized(this){
                // msg := "NET_ROUTE [%s] remoteAddress:%s, nextHop:%s, cost:%d\n"
                // log.Printf(msg, r.address, remoteAddress, nextHop, cost)
                if (info.cost == 0) { // zero cost routes are removed
                    if (info.address == this.address) { // fight the lie
                        Thread.sleep(500);
                        this.shareNetState();
                        return;
                    } 

                    // remove the route
                    if (this.remoteNodeMap.containsKey(info.address)) {
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
                    var ok bool
                    var remoteNode *RemoteNodeInfo
                    if remoteNode, ok = r.remoteNodeMap[remoteAddress]; !ok {
                        remoteNode = &RemoteNodeInfo{
                            Address: remoteAddress,
                        }
                        r.remoteNodeMap[remoteAddress] = remoteNode
                    }
                    remoteNode.LastSeen = time.Now().UTC()
                    if cost < remoteNode.Cost || remoteNode.Cost == 0 {
                        remoteNode.NextHop = nextHop
                        remoteNode.Channel = channel
                        remoteNode.Cost = cost
                        // relay update to other channels
                        p := makeNetworkRouteSharePacket(r.address, remoteAddress, cost+1)
                        for _, c := range r.channels {
                            if c != channel {
                                c.Send(p)
                            }
                        }
                    }
                }
            }

        case NET_SERVICE:
            if address, service, serviceLoad, err := parseNetworkServiceSharePacket(packet); err == nil {
                // fmt.Printf("NET_SERVICE node:'%s', service:'%s', load:%d\n", address, service, serviceLoad)
                r.mutex.Lock()
                defer r.mutex.Unlock()
                if _, ok := r.serviceLoadMap[service]; !ok {
                    r.serviceLoadMap[service] = NodeLoadMap{}
                }
                r.serviceLoadMap[service][address] = NodeLoad{
                    Load:     serviceLoad,
                    LastSeen: time.Now().UTC(),
                }
                for _, c := range r.channels {
                    if c != channel {
                        c.Send(packet)
                    }
                }
                if packetList, ok := r.serviceQueue[service]; ok {
                    log.Printf("sending %d packet(s) to '%s'", len(packetList), address)
                    if route, ok := r.remoteNodeMap[address]; ok {
                        for _, p := range packetList {
                            p.DestAddr = address
                            p.NextAddr = route.NextHop
                            channel.Send(p)
                        }
                    }
                    delete(r.serviceQueue, service)
                }
            } else {
                log.Printf("error parsing NET_SERVICE: %s\n", err.Error())
            }

        case NET_QUERY:
            for _, routePacket := range r.ExportRouteTable() {
                channel.Send(routePacket)
            }
            for _, servicePacket := range r.ExportServiceTable() {
                channel.Send(servicePacket)
            }
        }
    }

    public void addChannel(IChannel channel) {
        synchronized(this) {
            this.channels.add(channel);
        }

        channel.receive(new IPacketHandler(){
            @Override
            public void onPacket(Packet packet) {
                // TODO Auto-generated method stub
                if (packet.Net != 0) {
                    handleNetState(packet);
                } else {
                    send(packet);
                }
            }
        }, new IChannelCloseHandler(){
            @Override
            public void onChannelClosed(IChannel c) {
                // TODO Auto-generated method stub        
            }
        });
        // channel.Send(makeNetQueryPacket()) // TODO send a query on the new connection
    }

    public void removeChannel(IChannel ch) { }

    protected void removeAddress(String address) { }

    public void registerService(String service, IPacketHandler handler) { }

    public void unregisterService(String service) { }

    public ArrayList<Packet> exportRouteTable() {
        return null;
    }

    public int numChannels() {
        return 0;
    }

    public ArrayList<Packet> exportServiceTable() {
        return null;
    }

    public void shareNetState() {
        // TODO
    }
}
