from threading import Thread, Lock

package aln

import "fmt"

def makeNetQueryPacket:
	return Packet(netState: NET_QUERY)

def makeNetworkRouteSharePacket(srcAddr, destAddr, cost):
	data := writeINT16U(destAddr)
	data = append(data, writeINT16U(cost)...)
	return Packet(
		netState=NET_ROUTE,
		srcAddr=srcAddr,
		data=data,
	)

# returns dest, next-hop, cost, err
func parseNetworkRouteSharePacket(packet *Packet) (AddressType, AddressType, uint16, error) {
	if packet.NetState != NET_ROUTE {
		return 0, 0, 0, fmt.Errorf("parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")
	}
	if len(packet.Data) != AddressTypeSize+2 {
		return 0, 0, 0, fmt.Errorf("parseNetworkRouteSharePacket: len(packet.Data != %d", AddressTypeSize+2)
	}
	addr := bytesToAddressType(packet.Data[:AddressTypeSize])
	cost := bytesToINT16U(packet.Data[AddressTypeSize:])
	return addr, packet.srcAddr, cost, nil
}

func makeNetworkServiceSharePacket(hostAddr AddressType, serviceID, serviceLoad uint16) *Packet {
	data := bytesOfAddressType(hostAddr)
	data = append(data, bytesOfINT16U(serviceID)...)
	data = append(data, bytesOfINT16U(serviceLoad)...)
	return &Packet{
		NetState: NET_SERVICE,
		Data:     data,
	}
}

func parseNetworkServiceSharePacket(packet *Packet) (AddressType, uint16, uint16, error) {
	if packet.NetState != NET_SERVICE {
		return 0, 0, 0, fmt.Errorf("parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")
	}
	if len(packet.Data) != AddressTypeSize+4 {
		return 0, 0, 0, fmt.Errorf("parseNetworkRouteSharePacket: len(packet.Data != %d", AddressTypeSize+4)
	}
	addr := bytesToAddressType(packet.Data[:AddressTypeSize])
	serviceID := bytesToINT16U(packet.Data[AddressTypeSize : AddressTypeSize+2])
	serviceLoad := bytesToINT16U(packet.Data[AddressTypeSize+2:])
	return addr, serviceID, serviceLoad, nil
}


class Router:
    def __init__(self, address):
        self.address = address
        self.lock = Lock()
        self.channels = []
	    self.serviceMap = {}     # map[address]callback registered local node packet handlers
	    self.contextMap = {}     # serviceHandlerMap
	    self.remoteNodeMap = {}  # RemoteNodeMap // map[address][]RemoteNodes
	    self.serviceLoadMap = {} # ServiceLoadMap


    def select_service(self, serviceID):
        pass
    
    def send(self, packet):
		if packet.destAddr is None and packet.serviceID is not None:
			packet.destAddr = r.select_service(packet.serviceID)
		
		if packet.destAddr is r.address:
			if packet.serviceID in r.serviceMap:
				r.serviceMap[packet.serviceID](packet)
			elif packet.contextID in r.contextMap:
				r.contextMap[packet.contextID](packet)
			else:
				print("send err, service:" + packet.serviceID + " context:"+ packet.contextID + " not registered")
			
		elif packet.nextAddr == r.address or packet.nextAddr == None	 {
			if route, ok := r.remoteNodeMap[packet.destAddr]; ok {
				packet.srcAddr = r.address
				packet.nextAddr = route.NextHop
				route.Channel.Send(p)
			} else {
				return fmt.Errorf("no route for %d [%X]", packet.destAddr, packet.destAddr)
			}
		} else {
			return fmt.Errorf("packet is unroutable; no action taken")
		}
		return nil

    def register_context_handler(self, callback):
        pass

    def release_context(self, ctxID):
        pass

    def register_service(self, serviceID, handler):
        pass

    def export_routes(self):
        pass

    def export_services(self);
        pass

    def share_net_state(self):
		pass