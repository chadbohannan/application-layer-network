package aln

import "fmt"

func makeNetQueryPacket() *Packet {
	return &Packet{
		NetState: NET_QUERY,
	}
}

func makeNetworkRouteSharePacket(srcAddr, destAddr AddressType, cost uint16) *Packet {
	data := []byte{byte(len(destAddr))}
	data = append(data, []byte(destAddr)...)
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
		return "", "", 0, fmt.Errorf("parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE")
	}
	if len(packet.Data) == 0 {
		return "", "", 0, fmt.Errorf("parseNetworkRouteSharePacket: packet.Data is empty")
	}
	addrSize := int(packet.Data[0])
	if len(packet.Data) != addrSize+3 {
		return "", "", 0, fmt.Errorf("parseNetworkRouteSharePacket: len(packet.Data) is %d; expexted %d", len(packet.Data), addrSize+3)
	}
	addr := bytesToAddressType(packet.Data[1 : addrSize+1])
	cost := bytesToINT16U(packet.Data[1+addrSize:])
	return addr, packet.SrcAddr, cost, nil
}

func makeNetworkServiceSharePacket(hostAddr AddressType, serviceID, serviceLoad uint16) *Packet {
	data := []byte{byte(len(hostAddr))}
	data = append(data, bytesOfAddressType(hostAddr)...)
	data = append(data, bytesOfINT16U(serviceID)...)
	data = append(data, bytesOfINT16U(serviceLoad)...)
	return &Packet{
		NetState: NET_SERVICE,
		Data:     data,
	}
}

func parseNetworkServiceSharePacket(packet *Packet) (AddressType, uint16, uint16, error) {
	if packet.NetState != NET_SERVICE {
		return "", 0, 0, fmt.Errorf("parseNetworkServiceSharePacket: packet.NetState != NET_ROUTE")
	}
	addrSize := packet.Data[0]
	if len(packet.Data) != int(addrSize)+5 {
		return "", 0, 0, fmt.Errorf("parseNetworkServiceSharePacket: len(packet.Data) = %d; expected: %d", len(packet.Data), addrSize+5)
	}
	addr := bytesToAddressType(packet.Data[1 : addrSize+1])
	serviceID := bytesToINT16U(packet.Data[addrSize+1 : addrSize+3])
	serviceLoad := bytesToINT16U(packet.Data[addrSize+3:])
	return addr, serviceID, serviceLoad, nil
}
