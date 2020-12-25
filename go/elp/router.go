package elp

import "fmt"

/*
An ELP network is a set of routers connected by channels.
Nodes may be local or remote addresses on the network.
*/

// ChannelCostMap describes the link cost of available
//   channels to a Node.
type ChannelCostMap map[Channel]uint16

// RouteMap holds all the RouteRecord elements with the same Dest.
//   Is used to select a channel to send a packet from Router.
type RouteMap map[AddressType]ChannelCostMap

// AddressCostMap is the value for the Router ChannelMap
type AddressCostMap map[AddressType]uint16

// ChannelMap caches addresses accessible through a channel
//   It should match the routeMap of the a remote channel (on point-to-point links)
type ChannelMap map[Channel]AddressCostMap

// NodeCallbackMap maps local endpoint nodes to packet handlers
type NodeCallbackMap map[AddressType]func(*Packet)

// Router manages a set of channels and packet handlers
type Router struct {
	localNodeMap NodeCallbackMap // registered local node packet handlers
	routeMap     RouteMap
	channelMap   ChannelMap
	onError      func(string)
}

// NewRouter instantiates an applications ELP Router
func NewRouter(onError func(string)) *Router {
	return &Router{
		localNodeMap: make(NodeCallbackMap),
		routeMap:     make(RouteMap),
		channelMap:   make(ChannelMap),
		onError:      onError,
	}
}

// Send is the core routing function of Router. A packet is either expected
//  locally by a registered Node, or on a multi-hop route to it's destination.
func (r *Router) Send(p *Packet) error {
	if onPacket, ok := r.localNodeMap[p.DestAddr]; ok {
		onPacket(p)
	} else if routes, ok := r.routeMap[p.DestAddr]; ok {
		if len(routes) == 0 {
			return fmt.Errorf("empty route list for %d [%X]", p.DestAddr, p.DestAddr)
		}
		var c Channel
		minCost := uint16(9999)
		for _c, cost := range routes {
			if cost < minCost {
				c = _c
				cost = minCost
			}
		}
		c.Send(p)
	} else {
		return fmt.Errorf("no route for %d [%X]", p.DestAddr, p.DestAddr)
	}
	return nil
}

// AddChannel initializes
func (r *Router) AddChannel(channel Channel) {
	// add channel to channel index
	r.channelMap[channel] = AddressCostMap{}

	// define onPacket() to either handle link-state updates or route data
	channel.Receive(func(packet *Packet) {
		if (packet.ControlFlags & CF_LINKSTATE) == CF_LINKSTATE {
			switch packet.LinkState {
			case LinkStateRouteQuery:
				// TODO response by generating LinkStateNodeRouteCost from the routing data
			case LinkStateNodeRouteCost:
				switch AddressTypeSize {
				case 1:
					if packet.DataSize == 4 {
						address := AddressType(packet.Data[0])
						cost := bytesToINT16U(packet.Data[1:3])
						r.channelMap[channel][address] = cost
						r.routeMap[address][channel] = cost
					} else {
						// TODO log error condition
					}
				case 2:
					if packet.DataSize == 4 {
						address := AddressType(bytesToINT16U(packet.Data[:2]))
						cost := bytesToINT16U(packet.Data[2:4])
						r.channelMap[channel][address] = cost
						r.routeMap[address][channel] = cost
					} else {
						// TODO log error condition
					}
				case 4:
					if packet.DataSize == 4 {
						address := AddressType(bytesToINT32U(packet.Data[:4]))
						cost := bytesToINT16U(packet.Data[4:6])
						r.channelMap[channel][address] = cost
						r.routeMap[address][channel] = cost
					} else {
						// TODO log error condition
					}
				}
			}
		} else {
			r.Send(packet)
		}
	})
}

// RegisterNode adds a node to the network accessible in zero hops from this
//  router. Badd stuff happens if the address is not unique on the network.
func (r *Router) RegisterNode(address AddressType, onPacket func(*Packet)) {
	r.localNodeMap[address] = onPacket
}

// ListenTCP blocks forever and should be run as a go routine
func (r *Router) ListenTCP(address string, port int) {
	// TODO listen and wrap incomming connections in new channels
}

// ListenWebsocket blocks forever
func (r *Router) ListenWebsocket(address string, port int) {
	// TODO listen and wrap incomming connections in new channels
}
