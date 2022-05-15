from datetime import datetime
from threading import Lock, Thread
import random
import time
from .packet import Packet, readINT16U, writeINT16U

bytesToAddressType = str

def makeNetQueryPacket():
    return Packet(netState=Packet.NET_QUERY)

def byteLen(buff):
    return len(buff).to_bytes(1, 'big')

def makeNetworkRouteSharePacket(srcAddr, destAddr, cost):
    data = []
    data.extend(byteLen(destAddr))
    data.extend(bytes(destAddr, "utf-8"))
    data.extend(writeINT16U(cost))
    # data = byteLen(destAddr) + bytes(destAddr, "utf-8") + writeINT16U(cost)
    return Packet(netState=Packet.NET_ROUTE, srcAddr=srcAddr,data=data)

def parseNetworkRouteSharePacket(packet): # returns dest, next-hop, cost, err
    if packet.netState != Packet.NET_ROUTE:
        return (0, 0, 0, "parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")
    addrSize = int(packet.data[0])
    if len(packet.data) != (addrSize + 3):
        return (0, 0, 0, "parseNetworkRouteSharePacket: len(packet.Data) != (addrSize + 3)")
    addr = bytearray(packet.data[1:addrSize+1]).decode("utf-8")
    cost = readINT16U(packet.data[addrSize+1:addrSize+3])
    return addr, packet.srcAddr, cost, None

def makeNetworkServiceSharePacket(hostAddr, service, serviceLoad):
    data = []
    data.extend(byteLen(hostAddr))
    data.extend(bytes(hostAddr, "utf-8"))
    data.extend(byteLen(service))
    data.extend(bytes(service, "utf-8"))
    data.extend(writeINT16U(serviceLoad))
    return Packet(netState=Packet.NET_SERVICE, data=data)

# returns hostAddr, service, load, error
def parseNetworkServiceSharePacket(packet):
    if packet.netState != Packet.NET_SERVICE:
        return (0, 0, 0, "parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")

    if len(packet.data) < 4:
        return (0, 0, 0, "parseNetworkRouteSharePacket: payload too small")

    # TODO validate message length to avoid seg fault
    addrSize = int(packet.data[0])
    hostAddr = bytearray(packet.data[1:addrSize+1]).decode("utf-8")
    srvSize = int(packet.data[addrSize+1])
    service = bytearray(packet.data[addrSize+2:addrSize+2+srvSize]).decode("utf-8")
    serviceLoad = readINT16U(packet.data[addrSize+2+srvSize:])
    return hostAddr, service, serviceLoad, None

class RemoteNode:
    def __init__(self, address, nextHop, cost, channel, lastSeen=datetime.now()):
        self.address = address
        self.nextHop = nextHop
        self.cost = cost
        self.channel = channel
        self.lastSeen = lastSeen

class Router(Thread):
    def __init__(self, selector, address=0):
        super(Router, self).__init__()
        self.lock = Lock()
        self.selector = selector # top-level application event loop
        self.address = address   # TODO dynamic address allocation
        self.channels = []       # pool of all channels for flood propagation
        self.contextMap = {}     # serviceHandlerMap
        self.remoteNodeMap = {}  # RemoteNodeMap // map[address]RemoteNodes
        self.serviceMap = {}     # map[address]callback registered local service handlers
        self.serviceLoadMap = {} # ServiceLoadMap // map[service][address]load (remote services)
        self.serviceQueue = {}   # map[service]PacketList
        self.stop = False
        self.daemon = True

    def run(self):
        while not self.stop:
            events = self.selector.select()
            for key, mask in events:
                callback = key.data
                callback(key.fileobj, mask)

    def handle_netstate(self, channel, packet):
        if packet.netState == Packet.NET_ROUTE: #  neighbor is sharing it's routing table')
            remoteAddress, nextHop, cost, err = parseNetworkRouteSharePacket(packet)
            if err is not None:
                print("err:",err)
            elif cost == 0:
                # TODO remove route
                if remoteAddress == self.address: # readvertize self
                    time.sleep(0.1)
                    self.share_net_state()
                else: # remove route
                    del self.remoteNodeMap[remoteAddress]
                    for loadMap in self.serviceLoadMap.values():
                        del loadMap[address]
                    for ch in self.channels:
                        if ch != channel:
                             ch.send(packet)
            else:
                msg = "NET_ROUTE to:[{rem}] via:[{next}] cost:{cost}"
                # print(msg.format(rem=remoteAddress, next=nextHop, cost=cost))
                if remoteAddress not in self.remoteNodeMap:
                    remoteNode = RemoteNode(remoteAddress, nextHop, cost, channel)
                    self.remoteNodeMap[remoteAddress] = remoteNode
                else:
                    remoteNode = self.remoteNodeMap[remoteAddress]
                    if remoteNode.channel not in self.channels or cost < remoteNode.cost or remoteNode.cost == 0:
                        remoteNode.nextHop = nextHop
                        remoteNode.channel = channel
                        remoteNode.cost = cost
                        #  relay update to other channels
                        p = makeNetworkRouteSharePacket(self.address, remoteAddress, cost+1)
                        for chan in self.channels:
                            if chan is not channel:
                                chan.send(p)
        elif packet.netState == Packet.NET_SERVICE: # neighbor is sharing service load info
            address, service, serviceLoad, err = parseNetworkServiceSharePacket(packet)
            if err is not None:
                print("error parsing NET_SERVICE: {0}", err)
            else:
                print("NET_SERVICE node:{0}, service:{1}, load:{2}".format(address, service, serviceLoad))
                if service not in self.serviceLoadMap:
                    self.serviceLoadMap[service] = {}
                self.serviceLoadMap[service][address] = serviceLoad
                # first priority is to share the updates with neighbors
                for chan in self.channels:
                    if chan is not channel:
                        chan.send(packet)
                # second priority is to send queued packets waiting on service discovery
                if service in self.serviceQueue:
                    for packet in self.serviceQueue[service]:
                        if address in self.remoteNodeMap:
                            packet.destAddr = address
                            packet.nextAddr = self.remoteNodeMap[address].nextHop
                            print("sending queued packet for service {0} to host:{1} via:{2}".format(
                                service, packet.destAddr, packet.nextAddr))
                            channel.send(packet)
                        else:
                            print("NET ERROR no route for advertised service: ", service)
                    del(self.serviceQueue, service)
        elif packet.netState == Packet.NET_QUERY:     
            for routePacket in self.export_routes():
                channel.send(routePacket)
            for servicePacket in self.export_services():
                channel.send(servicePacket)

    def on_packet(self, channel, packet):
        if packet.netState:
            with self.lock:
                self.handle_netstate(channel, packet)
        else:
            self.send(packet)

    def remove_channel(self, channel):
        print("channel disconnected")
        with self.lock:
            self.channels.remove(channel)
            delete = [addr for addr in self.remoteNodeMap if self.remoteNodeMap[addr].channel == channel]
            for addr in delete:
                del self.remoteNodeMap[addr]
                for ch in self.channels:
                    ch.send(makeNetworkRouteSharePacket(self.address, addr, 0))

    def add_channel(self, channel):
        channel.on_close = self.remove_channel
        with self.lock:
            self.channels.append(channel)
            channel.listen(self.selector, self.on_packet)
            channel.send(makeNetQueryPacket())
    
    def send(self, packet):
        if packet.srcAddr == None:
            packet.srcAddr = self.address

        # print("send from {0} to {1} via {2}, service:{3}, ctxID:{4}, datalen:{5}".format(
        #     packet.srcAddr, packet.destAddr, packet.nextAddr, packet.service, packet.contextID, len(packet.data))
        # )

        packetHandler = None
        with self.lock:
            if packet.destAddr is None and packet.service is not None:
                packet.destAddr = self.select_service(packet.service)
                if not packet.destAddr:
                    if packet.service in self.serviceQueue:
                        self.serviceQueue[packet.service].append(packet)
                    else:
                        self.serviceQueue[packet.service] = [packet]
                        return "service {0} unavailable, packet queued".format(packet.service)
            
            if packet.destAddr == self.address:
                if packet.service in self.serviceMap:
                    packetHandler = self.serviceMap[packet.service]
                elif packet.contextID in self.contextMap:
                    packetHandler = self.contextMap[packet.contextID]
                else:
                    return ("send err, service:" + packet.service + " context:"+ packet.contextID + " not registered")

            elif packet.nextAddr == self.address or packet.nextAddr == None:
                if packet.destAddr in self.remoteNodeMap:
                    route = self.remoteNodeMap[packet.destAddr]
                    packet.srcAddr = self.address
                    packet.nextAddr = route.nextHop
                    route.channel.send(packet)
                else:
                    return "no route for " + str(packet.destAddr)
            else:
                return "packet is unroutable; no action taken"
        if packetHandler:
            packetHandler(packet)
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

    def register_service(self, service, handler):
        with self.lock:
            self.serviceMap[service] = handler

    def unregister_service(self, service):
        with self.lock:
            self.serviceMap.pol(service, None)

    def select_service(self, service):
        # return the address of service with lowest reported load or None
        if service in self.serviceMap:
            return self.address

        minLoad = 0
        remoteAddress = None
        if service in self.serviceLoadMap: 
            for addr in self.serviceLoadMap[service]:
                if remoteAddress is None or self.serviceLoadMap[service][addr] < minLoad:
                    minLoad = self.serviceLoadMap[service][addr]
                    remoteAddress = addr
        return remoteAddress

    def export_routes(self):
        # compose routing table as an array of packets
        # one local route, with a cost of 1
        # for each remote route, our cost and add 1
        routes = [makeNetworkRouteSharePacket(self.address, self.address, 1)]
        for remoteAddress in self.remoteNodeMap:
            # TODO filter expired nodes by lastSeen 
            remoteNode = self.remoteNodeMap[remoteAddress]
            routes.append(makeNetworkRouteSharePacket(self.address, remoteAddress, remoteNode.cost+1))
        return routes

    def export_services(self):
        # compose a list of packets encoding the service table of this node
        services = []
        for service in self.serviceMap:
            load = 0 # TODO measure load
            services.append(makeNetworkServiceSharePacket(self.address, service, load))
        for service in self.serviceLoadMap:
            loadMap = self.serviceLoadMap[service]
            for remoteAddress in self.loadMap: # TODO sort by increasing load (first tx'd is lowest load)
                load = self.loadMap[remoteAddress]
                # TODO filter expired or unroutable entries
                services.append(makeNetworkServiceSharePacket(remoteAddress, service, load))
        return services

    def share_net_state(self):
        for channel in self.channels:
            for routePacket in self.export_routes():
                channel.send(routePacket)
            for servicePacket in self.export_services():
                channel.send(servicePacket)

    def close(self):
        self.stop = True
        for channel in self.channels:
            channel.close()
