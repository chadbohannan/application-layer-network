package aln

import (
	"fmt"
	"math/rand"
	"sync"
	"time"
)

/*
An ALN is a set of routers connected by channels.
Nodes may be local or remote addresses on the network.
*/

// contextHandlerMap stores query response handers by contextID
type contextHandlerMap map[uint16]func(*Packet)

// serviceHandlerMap maps local endpoint nodes to packet handlers
type serviceHandlerMap map[uint16]func(*Packet)

type RemoteNodeInfo struct {
	Address  AddressType // target addres to communicate with
	NextHop  AddressType // next routing service node for address
	Channel  Channel     // specific channel that is hosting next hop
	Cost     uint16      // cost of using this route, generally a hop count
	LastSeen time.Time   // routes should decay with a few missed updates
}

type RemoteNodeMap map[AddressType]*RemoteNodeInfo

type NodeLoad struct {
	Load     uint16
	LastSeen time.Time
}

type NodeLoadMap map[AddressType]NodeLoad  // map[adress]NodeLoad
type ServiceLoadMap map[uint16]NodeLoadMap // map[serviceID][address]NodeLoad

// Router manages a set of channels and packet handlers
type Router struct {
	mutex          sync.Mutex
	address        AddressType
	channels       []Channel
	serviceMap     serviceHandlerMap // map[address]callback registered local node packet handlers
	contextMap     serviceHandlerMap
	remoteNodeMap  RemoteNodeMap // map[address][]RemoteNodes
	serviceLoadMap ServiceLoadMap
}

// NewRouter instantiates an applications ELP Router
func NewRouter(address AddressType) *Router {
	return &Router{
		address:        address,
		channels:       []Channel{},
		serviceMap:     make(serviceHandlerMap),
		contextMap:     make(serviceHandlerMap),
		remoteNodeMap:  make(RemoteNodeMap),
		serviceLoadMap: make(ServiceLoadMap),
	}
}

// Send is the core routing function of Router. A packet is either expected
//  locally by a registered Node, or on a multi-hop route to it's destination.
func (r *Router) Send(p *Packet) error {
	if p.DestAddr == r.address {
		if onPacket, ok := r.serviceMap[p.ServiceID]; ok {
			onPacket(p)
		} else if onPacket, ok = r.contextMap[p.ContextID]; ok {
			onPacket(p)
		} else {
			return fmt.Errorf("service %d not registered", p.ServiceID)
		}
	} else if p.NextAddr == r.address || p.NextAddr == 0 {
		if route, ok := r.remoteNodeMap[p.DestAddr]; ok {
			p.SrcAddr = r.address
			p.NextAddr = route.NextHop
			route.Channel.Send(p)
		} else {
			return fmt.Errorf("no route for %d [%X]", p.DestAddr, p.DestAddr)
		}
	}
	return nil
}

func (r *Router) RegisterContextHandler(packetHandler func(*Packet)) uint16 {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	ctxID := uint16(rand.Intn(2<<15 + 1))
	for _, ok := r.contextMap[ctxID]; ok; {
		ctxID = uint16(rand.Intn(2<<15 + 1))
	}
	r.contextMap[ctxID] = packetHandler
	return ctxID
}

func (r *Router) ReleaseContext(ctxID uint16) {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	delete(r.contextMap, ctxID)
}

// AddChannel initializes
func (r *Router) AddChannel(channel Channel) {
	r.mutex.Lock()
	r.channels = append(r.channels, channel)
	r.mutex.Unlock()

	// define onPacket() to either handle link-state updates or route data
	go channel.Receive(func(packet *Packet) {
		if (packet.ControlFlags & CF_NETSTATE) == CF_NETSTATE {
			switch packet.NetState {
			case NET_ROUTE:
				// neighbor is sharing it's routing table
				if remoteAddress, nextHop, cost, err := parseNetworkRouteSharePacket(packet); err == nil {
					// msg := "NET_ROUTE [%d] remoteAddress:%d, nextHop:%d, cost:%d\n"
					// fmt.Printf(msg, r.address, remoteAddress, nextHop, cost)
					r.mutex.Lock()
					defer r.mutex.Unlock()
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
				} else {
					fmt.Printf("error parsing NET_ROUTE: %s", err.Error())
				}

			case NET_SERVICE:
				if address, serviceID, serviceLoad, err := parseNetworkServiceSharePacket(packet); err == nil {
					// fmt.Printf("NET_SERVICE node:%d, service:%d, load:%d\n", address, serviceID, serviceLoad)
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

			case NET_QUERY:
				for _, routePacket := range r.ExportRouteTable() {
					channel.Send(routePacket)
				}
			}
		} else {
			r.Send(packet)
		}
	})
	channel.Send(makeNetQueryPacket())
}

// RegisterService adds a node to the network accessible in zero hops from this
//  router. Badd stuff happens if the address is not unique on the network.
func (r *Router) RegisterService(serviceID uint16, onPacket func(*Packet)) {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	r.serviceMap[serviceID] = onPacket
	go r.ShareRoutes()
}

// UnregisterService is a cleanup mechanism for elegant application teardown
func (r *Router) UnregisterService(serviceID uint16) {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	delete(r.serviceMap, serviceID)
}

// ExportRouteTable generates a set of packets that another router can read
func (r *Router) ExportRouteTable() []*Packet {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	// compose routing table as an array of packets
	// one local route, with a cost of 1
	// for each remote route, our cost and add 1
	routes := []*Packet{makeNetworkRouteSharePacket(r.address, r.address, 1)}
	for address, remoteNode := range r.remoteNodeMap {
		// TODO filter by LastSeen age
		routes = append(routes, makeNetworkRouteSharePacket(r.address, address, remoteNode.Cost+1))
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
