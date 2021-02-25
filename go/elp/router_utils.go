package elp

import "fmt"

func makeNetQueryPacket() *Packet {
	return &Packet{
		NetState: NET_QUERY,
	}
}

func makeNetworkRouteSharePacket(srcAddr, destAddr AddressType, cost uint16) *Packet {
	data := bytesOfAddressType(destAddr)
	data = append(data, bytesOfINT16U(cost)...)
	return &Packet{
		NetState: NET_ROUTE,
		SrcAddr:  srcAddr,
		Data:     data,
	}
}

// returns dest, next-hop, cost, err
func parseNetworkRouteSharePacket(packet *Packet) (AddressType, AddressType, uint16, error) {
	if packet.NetState != NET_ROUTE {
		return 0, 0, 0, fmt.Errorf("parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")
	}
	if len(packet.Data) != AddressTypeSize+2 {
		return 0, 0, 0, fmt.Errorf("parseNetworkRouteSharePacket: len(packet.Data != %d", AddressTypeSize+2)
	}
	addr := bytesToAddressType(packet.Data[:AddressTypeSize])
	cost := bytesToINT16U(packet.Data[AddressTypeSize:])
	return addr, packet.SrcAddr, cost, nil
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
