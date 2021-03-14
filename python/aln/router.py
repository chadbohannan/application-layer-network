from threading import Lock
import random
from .packet import Packet

def makeNetQueryPacket():
    return Packet(netState=Packet.NET_QUERY)

def makeNetworkRouteSharePacket(srcAddr, destAddr, cost):
    data = writeINT16U(destAddr) + writeINT16U(cost)
    return Packet(netState=Packet.NET_ROUTE, srcAddr=srcAddr,data=data)

def parseNetworkRouteSharePacket(packet): # returns dest, next-hop, cost, err
    if packet.netState != Packet.NET_ROUTE:
        return (0, 0, 0, "parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")
    if len(packet.data) != 4:
        return (0, 0, 0, "parseNetworkRouteSharePacket: len(packet.Data) != 4")
    
    addr = readINT16U(packet.Data[:2])
    cost = readINT16U(packet.Data[2:])
    return addr, packet.srcAddr, cost, None

def makeNetworkServiceSharePacket(hostAddr, serviceID, serviceLoad):
    data = writeINT16U(hostAddr) + \
        writeINT16U(serviceID) + \
        writeINT16U(serviceLoad)
    return Packet(netState=NET_SERVICE, data=data)

# returns hostAddr, serviceID, load, error
def parseNetworkServiceSharePacket(packet):
    if packet.netState != Packet.NET_SERVICE:
        return (0, 0, 0, "parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")

    if len(packet.Data) != AddressTypeSize+4:
        return (0, 0, 0, "parseNetworkRouteSharePacket: len(packet.Data != 6")

    hostAddr = bytesToAddressType(packet.Data[:AddressTypeSize])
    serviceID = readINT16U(packet.Data[AddressTypeSize : AddressTypeSize+2])
    serviceLoad = readINT16U(packet.Data[AddressTypeSize+2:])
    return hostAddr, serviceID, serviceLoad, None

class RemoteNode:
    def __init__(self, address, nextHop, cost, channel, lastSeen):
        self.address = address
        self.nextHop = nextHop
        self.cost = cost
        self.channel = channel
        self.lastSeen = lastSeen

class Router:
    def __init__(self, selector, address=0): # zero address indicates address-request is required
        self.selector = selector # top-level application event loop
        self.address = address
        self.lock = Lock()
        self.channels = []
        self.serviceMap = {}     # map[address]callback registered local node packet handlers
        self.contextMap = {}     # serviceHandlerMap
        self.remoteNodeMap = {}  # RemoteNodeMap // map[address]RemoteNodes
        self.serviceLoadMap = {} # ServiceLoadMap // map[serviceID][address]load

    def handle_netstate(self, channel, packet):
        if packet.netState is Packet.NET_ROUTE: #  neighbor is sharing it's routing table
            remoteAddress, nextHop, cost, err = parseNetworkRouteSharePacket(packet)
            if err is not None:
                print(err)
            else:
                msg = "NET_ROUTE [{addr}] remoteAddress:{rem}, nextHop:{next}, cost:{cost}"
                print(msg.format(addr=r.address, rem=remoteAddress, next=nextHop, cost=cost))
               
                remoteNode = None
                if remoteAddress not in self.remoteNodeMap:
                    remoteNode = RemoteNode(remoteaddress, nextHop, cost, channel)
                else:
                    remoteNode = self.remoteNodeMap[remoteAddress]
                    if cost < remoteNode.cost or remoteNode.Cost is 0:
                        remoteNode.NextHop = nextHop
                        remoteNode.Channel = channel
                        remoteNode.Cost = cost
                        #  relay update to other channels
                        p = makeNetworkRouteSharePacket(self.address, remoteAddress, cost+1)
                        for chan in self.channels:
                            if chan is not channel:
                                chan.send(p)

        if packet.netState is Packet.NET_SERVICE: # neighbor is sharing service load info
            address, serviceID, serviceLoad, err = parseNetworkServiceSharePacket(packet)
            if err is not None:
                print("error parsing NET_SERVICE: {0}", err)
            else:
                print("NET_SERVICE node:{0}, service:{1}, load:{2}\n".format(address, serviceID, serviceLoad))
                if serviceID not in self.serviceLoadMap:
                    self.serviceLoadMap[serviceID] = {}
                self.serviceLoadMap[serviceID][address] = serviceLoad

                for chan in self.channels:
                    if chan is not channel:
                        chan.send(packet)

        if packet.netState is Packet.NET_QUERY:
            for routePacket in self.export_routes():
                channel.Send(routePacket)
            for servicePacket in self.export_services():
                channel.Send(servicePacket)

    def on_packet(self, channel, packet):
        if packet.controlFlags & Packet.CF_NETSTATE == Packet.CF_NETSTATE:
            with self.lock:
                self.handle_netstate(channel, packet)
        else:
            self.send(packet)

    def add_channel(self, channel):
        with self.lock:
            self.channels.append(channel)
            channel.listen(self.selector, self.on_packet)
            # TODO send NET_QUERY
    
    def send(self, packet):
        # TODO lock
        if packet.destAddr is None and packet.serviceID is not None:
            packet.destAddr = r.select_service(packet.serviceID)
        
        if packet.destAddr is r.address:
            if packet.serviceID in r.serviceMap:
                r.serviceMap[packet.serviceID](packet)
            elif packet.contextID in r.contextMap:
                r.contextMap[packet.contextID](packet)
            else:
                return ("send err, service:" + packet.serviceID + " context:"+ packet.contextID + " not registered")
            
        elif packet.nextAddr == r.address or packet.nextAddr == None:
            if packet.destAddr in self.remoteNodeMap:
                route = self.remoteNodeMap[packet.destAddr]
                packet.srcAddr = self.address
                packet.nextAddr = route.nextHop
                route.channel.send(packet)
            else:
                return "no route for " + packet.destAddr
        else:
            return "packet is unroutable; no action taken"
        return None

    def register_context_handler(self, callback):
        with self.lock:
            ctxID = random.randint(2, 65535)
            while ctxID in self.contextMap:
                ctxID = random.randint(2, 65535)
            self.contextMap[ctxID] = callback
            return ctxID

    def release_context(self, ctxID):
        with self.lock:
            self.contextMap.pop(ctxID, None)

    def register_service(self, serviceID, handler):
        with self.lock:
            self.serviceHandlerMap[serviceID] = handler

    def unregister_service(elf, serviceID):
        with self.lock:
            self.serviceHandlerMap.pol(serviceID, None)

    def select_service(self, serviceID):
        # TODO return address of service with lowest reported load
        return 0

    def export_routes(self):
        # TODO return packets
        return []

    def export_services(self):
        # TODO return packets
        return []
