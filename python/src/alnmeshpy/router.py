import random
from datetime import datetime
from threading import Lock, Thread
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

def makeNetworkServiceSharePacket(hostAddr, service, serviceCapacity):
    data = []
    data.extend(byteLen(hostAddr))
    data.extend(bytes(hostAddr, "utf-8"))
    data.extend(byteLen(service))
    data.extend(bytes(service, "utf-8"))
    data.extend(writeINT16U(serviceCapacity))
    return Packet(netState=Packet.NET_SERVICE, data=data)

# returns hostAddr, service, capacity, error
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
    capacity = readINT16U(packet.data[addrSize+2+srvSize:])
    return hostAddr, service, capacity, None

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
        self.address = address   # network node ID for this router
        self.channels = []       # pool of all channels for flood propagation
        self.contextMap = {}     # serviceHandlerMap
        self.remoteNodeMap = {}  # RemoteNodeMap // map[address]RemoteNodes
        self.serviceMap = {}     # map[address]callback registered local service handlers
        self.serviceCapacityMap = {} # ServiceCapacityMap // map[service][address]capacity (remote services)
        self.serviceQueue = {}   # map[service]PacketList
        self.onServiceCapacityChanged = None # callback method (optional)
        self._stop_flag = False
        self.daemon = True

    def run(self):
        while not self._stop_flag:
            try:
                events = self.selector.select(timeout=0.1)
                for key, mask in events:
                    callback = key.data
                    callback(key.fileobj, mask)
            except (OSError, ValueError):
                # Selector closed or invalid, exit gracefully
                break

    def stop(self):
        self._stop_flag = True

    def handle_netstate(self, channel, packet):
        updatedServiceCapacity = None
        if packet.netState == Packet.NET_ROUTE: #  neighbor is sharing its routing table')
            remoteAddress, nextHop, cost, err = parseNetworkRouteSharePacket(packet)
            # msg = "recvd NET_ROUTE at [{address}] to:[{rem}] via:[{next}] cost:{cost}"
            # print(msg.format(address=self.address, rem=remoteAddress, next=nextHop, cost=cost))
            if err is not None:
                print("err:",err)
            elif cost == 0: # remove route
                if remoteAddress == self.address: # readvertize self
                    # print('sharing net state at {0}to refresh zero cost route discovery'.format(self.address))
                    self.share_net_state()
                elif remoteAddress in self.remoteNodeMap: # remove route
                    # print("removing route at {0} to {1}".format(self.address, remoteAddress))
                    if remoteAddress in self.remoteNodeMap:
                        del self.remoteNodeMap[remoteAddress]
                    for capacityMap in self.serviceCapacityMap.values():
                        if remoteAddress in capacityMap:
                            del capacityMap[remoteAddress]
                    for ch in self.channels:
                        if ch != channel:
                             ch.send(packet)
            else:
                if remoteAddress not in self.remoteNodeMap:
                    # print("new node discovered at {0}:{1}".format(self.address,  remoteAddress))
                    remoteNode = RemoteNode(remoteAddress, nextHop, cost, channel)
                    self.remoteNodeMap[remoteAddress] = remoteNode
                    p = makeNetworkRouteSharePacket(self.address, remoteAddress, cost+1)
                    for chan in self.channels:
                        if chan is not channel:
                            chan.send(p)
                else:
                    remoteNode = self.remoteNodeMap[remoteAddress]
                    if remoteNode.channel not in self.channels or cost < remoteNode.cost or remoteNode.cost == 0:
                        # print('updating route at {0} to {1}'.format(self.address, remoteAddress))
                        remoteNode.nextHop = nextHop
                        remoteNode.channel = channel
                        remoteNode.cost = cost
                        #  relay update to other channels
                        p = makeNetworkRouteSharePacket(self.address, remoteAddress, cost+1)
                        for chan in self.channels:
                            if chan is not channel:
                                chan.send(p)
                    else:
                        pass
                        # print('no action taken at {0} for route to {1}'.format(self.address, remoteAddress))
        elif packet.netState == Packet.NET_SERVICE: # neighbor is sharing service capacity info
            address, service, capacity, err = parseNetworkServiceSharePacket(packet)
            # print("recvd NET_SERVICE at {0} for: {1}, service:{2}, capacity: {3}".format(self.address, address, service, capacity))
            if err is not None:
                print("error parsing NET_SERVICE at {0}: {0}".format(self.address, err))
            elif address != self.address:
                if service not in self.serviceCapacityMap:
                    self.serviceCapacityMap[service] = {} # ensure an entry exists for this service name
                
                if capacity == 0 and address in self.serviceCapacityMap[service]: # remove zero capacity services from the service map
                    del self.serviceCapacityMap[service][address]
                    updatedServiceCapacity = (service, 0)
                    updatedServiceCapacity = (service, 0, address)
                    print("clearing service at {0} for {1}:{2}".format(self.address, address, service))

                if capacity != 0 and address not in self.serviceCapacityMap[service] or \
                    self.serviceCapacityMap[service][address] != capacity: # skip if no change
                        # print("updating service at {0} for {1}:{2}:{3}".format(self.address, address, service, capacity))
                        self.serviceCapacityMap[service][address] = capacity
                        # share the update with neighbors
                        for chan in self.channels:
                            if chan is not channel:
                                # print("sharing service at {0} for {1}:{2}".format(self.address, address, service))
                                chan.send(packet)
                        updatedServiceCapacity = (service, capacity, address)
                else:
                    pass
                    # print("no action for service at {0} for {1}:{2}:{3}".format(self.address, address, service, capacity))
        elif packet.netState == Packet.NET_QUERY:
            self.share_net_state()
        return updatedServiceCapacity
    
    def on_packet(self, channel, packet):
        # print("node {0} recieved packet with netState:{1}".format(self.address, packet.netState))
        if packet.netState:
            capacityUpdate = None
            with self.lock:
                capacityUpdate = self.handle_netstate(channel, packet)
            if capacityUpdate:
                # print("service capacity update at {0}, {1}".format(self.address, capacityUpdate))
                self._on_service_capacity_changed(capacityUpdate[0], capacityUpdate[1], capacityUpdate[2])
        else:
            self.send(packet)

    def remove_channel(self, channel):
        with self.lock:
            self.channels.remove(channel)
            delete = [addr for addr in self.remoteNodeMap if self.remoteNodeMap[addr].channel == channel]
            for addr in delete:
                del self.remoteNodeMap[addr]
                for ch in self.channels:
                    ch.send(makeNetworkRouteSharePacket(self.address, addr, 0))

    def add_channel(self, channel):
        channel.on_close(self.remove_channel)
        with self.lock:
            self.channels.append(channel)
        channel.listen(self.selector, self.on_packet)
        # TODO delayed query
        # self.loop.call_later(0.1, lambda channel: channel.send(makeNetQueryPacket()))
        channel.send(makeNetQueryPacket())
    
    def send(self, packet):
        if packet.srcAddr == None:
            packet.srcAddr = self.address

        # print("router.send from {0} to {1} via {2}, service:{3}, ctxID:{4}, datalen:{5}, data:{6}".format(
        #     packet.srcAddr, packet.destAddr, packet.nextAddr, packet.service, packet.contextID, len(packet.data), packet.data)
        # )

        packetHandler = None
        if packet.destAddr is None and packet.service is not None:
            addresses = self.service_addresses(packet.service)
            if len(addresses):
                # print("service " + packet.service + " is available")
                for i in range(1, len(addresses)):
                    pc = packet.copy()
                    pc.destAddr = addresses[i]
                    self.send(pc)
                packet.destAddr = addresses[0]
            else :
                return ("service of type " + packet.service + " not yet discovered on network")

        with self.lock:
            if packet.destAddr == self.address:
                if packet.service in self.serviceMap:
                    packetHandler = self.serviceMap[packet.service]
                elif packet.contextID in self.contextMap:
                    packetHandler = self.contextMap[packet.contextID]
                else:
                    return ("no suitable handler found for inbound packet")
            elif packet.nextAddr == self.address or packet.nextAddr == None:
                if packet.destAddr in self.remoteNodeMap:
                    # print('route available at', self.address, 'to', packet.destAddr)
                    route = self.remoteNodeMap[packet.destAddr]
                    packet.nextAddr = route.nextHop
                    route.channel.send(packet)
                else:
                    # print("route from {0} to {1} is unavailable".format(self.address, packet.destAddr))
                    return "no route for " + str(packet.destAddr)
            else:
                return "packet is unroutable; no action taken"
        if packetHandler:
            packetHandler(packet)
        return None

    def set_on_service_capacity_changed_handler(self, callback):
        self.onServiceCapacityChanged = callback
    
    def _on_service_capacity_changed(self, service, capacity, addr):
        if self.onServiceCapacityChanged:
            self.onServiceCapacityChanged(service, capacity, addr);

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
        # TODO isolate sharing routing table from sharing service mappings
        self.share_net_state()

    def unregister_service(self, service):
        with self.lock:
            self.serviceMap.pop(service, None)
        # TODO send service offline packet
        self.share_net_state()

    def service_addresses(self, service):
        addresses = []
        with self.lock:            
            if service in self.serviceMap:
                addresses.append(self.address)
            if service in self.serviceCapacityMap: 
                for addr in self.serviceCapacityMap[service]:
                    addresses.append(addr)
        return addresses

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
            capacity = 1 # TODO make local capacity variable
            services.append(makeNetworkServiceSharePacket(self.address, service, capacity))
        for service in self.serviceCapacityMap:
            capacityMap = self.serviceCapacityMap[service]
            for remoteAddress in capacityMap: # TODO sort by deccreasing capacity (first tx'd is highest capacity)
                # TODO filter expired or unroutable entries
                capacity = capacityMap[remoteAddress]
                services.append(makeNetworkServiceSharePacket(remoteAddress, service, capacity))
        return services

    def share_net_state(self):
        routes = self.export_routes()
        services = self.export_services()
        # print('node {0} sharing {1} routes, and {2} services over {3} channels'.format(
        #     self.address, len(routes), len(services), len(self.channels)))
        for channel in self.channels:
            for routePacket in routes:
                channel.send(routePacket)
            for servicePacket in services:
                channel.send(servicePacket)

    def close(self):
        self.stop = True
        for channel in self.channels:
            channel.close()
