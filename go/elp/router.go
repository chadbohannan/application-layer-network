package elp

import (
	"fmt"
	"sync"
	"time"
)

/*
An ELP network is a set of routers connected by channels.
Nodes may be local or remote addresses on the network.
*/

// serviceHandlerMap maps local endpoint nodes to packet handlers
type serviceHandlerMap map[uint16]func(*Packet)

type RemoteNode struct {
	Address  AddressType // target addres to communicate with
	NextHop  AddressType // next routing service node for address
	Channel  Channel     // specific channel that is hosting next hop
	Cost     uint16      // cost of using this route, generally a hop count
	LastSeen time.Time   // routes should decay with a few missed updates
}

type RemoteNodeList []RemoteNode

func (r RemoteNodeList) Len() int      { return len(r) }
func (r RemoteNodeList) Swap(i, j int) { r[i], r[j] = r[j], r[i] }
func (r RemoteNodeList) Less(i, j int) bool {
	if r[i].Cost != r[j].Cost {
		return r[i].Cost < r[j].Cost
	}
	return r[i].LastSeen.Before(r[j].LastSeen)
}

type RemoteNodeMap map[AddressType]RemoteNodeList

type NodeLoad struct {
	Load     uint16
	LastSeen time.Time
}

type NodeLoadMap map[AddressType]NodeLoad  // map[adress]NodeLoad
type ServiceLoadMap map[uint16]NodeLoadMap // map[serviceID][address]NodeLoad

// Router manages a set of channels and packet handlers
type Router struct {
	mutex           sync.Mutex
	address         AddressType
	channels        []Channel
	localServiceMap serviceHandlerMap // map[address]callback registered local node packet handlers
	remoteNodeMap   RemoteNodeMap     // map[address][]RemoteNodes
	serviceLoadMap  ServiceLoadMap
	onError         func(string)
}

// NewRouter instantiates an applications ELP Router
func NewRouter(address AddressType, onError func(string)) *Router {
	return &Router{
		address:         address,
		channels:        []Channel{},
		localServiceMap: make(serviceHandlerMap),
		remoteNodeMap:   make(RemoteNodeMap),
		serviceLoadMap:  make(ServiceLoadMap),
		onError:         onError,
	}
}

// Send is the core routing function of Router. A packet is either expected
//  locally by a registered Node, or on a multi-hop route to it's destination.
func (r *Router) Send(p *Packet) error {
	p.SrcAddr = r.address
	if p.DestAddr == r.address {
		if onPacket, ok := r.localServiceMap[p.ServiceID]; ok {
			onPacket(p)
		} else {
			r.onError(fmt.Sprintf("service %d not registered", p.ServiceID))
		}
	} else if p.NextAddr == r.address || p.NextAddr == 0 {
		if routes, ok := r.remoteNodeMap[p.DestAddr]; ok {
			if len(routes) == 0 {
				return fmt.Errorf("empty route list for %d [%X]", p.DestAddr, p.DestAddr)
			}
			var c Channel
			var cost uint16
			var nextHop AddressType
			for _, route := range routes {
				if cost == 0 || route.Cost < cost {
					c = route.Channel
					cost = route.Cost
					nextHop = route.NextHop
				}
			}
			if c != nil {
				p.NextAddr = nextHop
				c.Send(p)
			} else {
				return fmt.Errorf("empty route list for for %d [%X]", p.DestAddr, p.DestAddr)
			}
		} else {
			return fmt.Errorf("no routes for %d [%X]", p.DestAddr, p.DestAddr)
		}
	}
	return nil
}

// AddChannel initializes
func (r *Router) AddChannel(channel Channel) {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	// cache channel for routing and service table syncronization
	r.channels = append(r.channels, channel)

	// define onPacket() to either handle link-state updates or route data
	go channel.Receive(func(packet *Packet) {

		if (packet.ControlFlags & CF_NETSTATE) == CF_NETSTATE {
			switch packet.NetState {
			case NET_ROUTE:
				// neighbor is sharing it's routing table
				if remoteAddress, nextHop, cost, err := parseNetworkRouteSharePacket(packet); err == nil {
					fmt.Printf("NET_ROUTE remoteAddress:%d, nextHop:%d, cost:%d\n", remoteAddress, nextHop, cost)
					r.mutex.Lock()
					defer r.mutex.Unlock()
					var ok bool
					var remoteNodes RemoteNodeList
					if remoteNodes, ok = r.remoteNodeMap[remoteAddress]; !ok {
						remoteNodes = RemoteNodeList{}
					}
					r.remoteNodeMap[remoteAddress] = append(remoteNodes, RemoteNode{
						Address:  remoteAddress,
						NextHop:  nextHop,
						Channel:  channel,
						Cost:     cost,
						LastSeen: time.Now().UTC(),
					})
				} else {
					fmt.Printf("error parsing NET_ROUTE: %s", err.Error())
				}

			case NET_SERVICE:
				if address, serviceID, serviceLoad, err := parseNetworkServiceSharePacket(packet); err == nil {
					fmt.Printf("NET_SERVICE node:%d, service:%d, load:%d\n", address, serviceID, serviceLoad)
					r.mutex.Lock()
					defer r.mutex.Unlock()
					if _, ok := r.serviceLoadMap[serviceID]; !ok {
						r.serviceLoadMap[serviceID] = NodeLoadMap{}
					}
					r.serviceLoadMap[serviceID][address] = NodeLoad{
						Load:     serviceLoad,
						LastSeen: time.Now().UTC(),
					}
				} else {
					fmt.Printf("error parsing NET_SERVICE: %s", err.Error())
				}
			}
		} else {
			r.Send(packet)
		}
	})
}

// RegisterService adds a node to the network accessible in zero hops from this
//  router. Badd stuff happens if the address is not unique on the network.
func (r *Router) RegisterService(serviceID uint16, onPacket func(*Packet)) {
	r.mutex.Lock()
	r.localServiceMap[serviceID] = onPacket
	r.mutex.Unlock()
	r.ShareRoutes()
}

// UnregisterService is a cleanup mechanism for elegant application teardown
func (r *Router) UnregisterService(serviceID uint16) {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	delete(r.localServiceMap, serviceID)
}

// ExportRouteTable generates a set of packets that another router can read
func (r *Router) ExportRouteTable() []*Packet {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	// compose routing table as an array of packets
	// one local route, with a cost of 1
	// for each remote route, select cheapest and add 1
	routes := []*Packet{makeNetworkRouteSharePacket(r.address, r.address, 1)}
	for address, remoteNodes := range r.remoteNodeMap {
		var cost uint16
		for _, remoteNode := range remoteNodes {
			if cost == 0 || remoteNode.Cost < cost {
				cost = remoteNode.Cost
			}
		}
		if cost > 0 {
			routes = append(routes, makeNetworkRouteSharePacket(r.address, address, cost+1))
		} else {
			// TODO clean up empty target list
		}
	}
	return routes
}

// ShareRoutes broadcasts the local routing table
func (r *Router) ShareRoutes() {
	routes := r.ExportRouteTable()
	r.mutex.Lock()
	defer r.mutex.Unlock()
	for _, channel := range r.channels {
		for _, routePacket := range routes {
			channel.Send(routePacket)
		}
	}
}
