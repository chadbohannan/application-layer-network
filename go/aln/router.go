package aln

import (
	"fmt"
	"log"
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
	serviceQueue   map[uint16][]*Packet // packets waiting for services during warmup
}

// NewRouter instantiates an applications ELP Router
func NewRouter(address AddressType) *Router {
	return &Router{
		address:        address,
		channels:       make([]Channel, 0),
		serviceMap:     make(serviceHandlerMap),
		contextMap:     make(serviceHandlerMap),
		remoteNodeMap:  make(RemoteNodeMap),
		serviceLoadMap: make(ServiceLoadMap),
		serviceQueue:   make(map[uint16][]*Packet),
	}
}

// SelectService returns the address of the service with the lowest load, or zero
func (r *Router) SelectService(serviceID uint16) AddressType {
	if _, ok := r.serviceMap[serviceID]; ok {
		return r.address
	}
	var minLoad uint16
	var remoteAddress AddressType
	if loadMap, ok := r.serviceLoadMap[serviceID]; ok {
		for address, load := range loadMap {
			if minLoad == 0 || minLoad > load.Load {
				remoteAddress = address
				minLoad = load.Load
			}
		}
	}
	return remoteAddress
}

// Send is the core routing function of Router. A packet is either expected
//  locally by a registered Node, or on a multi-hop route to it's destination.
func (r *Router) Send(p *Packet) error {

	handler := func(p *Packet) {}
	packetCallback := func(p *Packet) { handler(p) }
	defer packetCallback(p)

	r.mutex.Lock()
	defer r.mutex.Unlock()

	if len(p.SrcAddr) == 0 {
		p.SrcAddr = r.address
	}
	if len(p.DestAddr) == 0 && p.ServiceID != 0 {
		p.DestAddr = r.SelectService(p.ServiceID)
		if len(p.DestAddr) == 0 {
			if _, ok := r.serviceQueue[p.ServiceID]; !ok {
				r.serviceQueue[p.ServiceID] = []*Packet{p}
			} else {
				r.serviceQueue[p.ServiceID] = append(r.serviceQueue[p.ServiceID], p)
			}
			return fmt.Errorf("serviceID %d unavailable, packet queued", p.ServiceID)
		}
	}
	if p.DestAddr == r.address {
		if onPacket, ok := r.serviceMap[p.ServiceID]; ok {
			handler = onPacket
		} else if onPacket, ok = r.contextMap[p.ContextID]; ok {
			handler = onPacket
		} else {
			return fmt.Errorf("service %d not registered", p.ServiceID)
		}
	} else if p.NextAddr == r.address || len(p.NextAddr) == 0 {
		if route, ok := r.remoteNodeMap[p.DestAddr]; ok {
			p.NextAddr = route.NextHop
			route.Channel.Send(p)
		} else {
			return fmt.Errorf("packet queued for delayed send t %s [%X]", p.DestAddr, p.DestAddr)
		}
	} else {
		return fmt.Errorf("packet is unroutable; no action taken")
	}
	return nil
}

// RegisterContextHandler returns a contextID. Services must respond with the same
// contextID to reach the correct response handler.
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

// ReleaseContext frees memory associated with a context
func (r *Router) ReleaseContext(ctxID uint16) {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	delete(r.contextMap, ctxID)
}

func (r *Router) RemoveChannel(channel Channel) {
	log.Printf("router:RemoveChannel")
	go func() {
		r.mutex.Lock()
		defer r.mutex.Unlock()
		// slice out the channel
		for i, ch := range r.channels {
			if ch == channel {
				a := r.channels
				a[i] = a[len(a)-1]
				a[len(a)-1] = nil
				r.channels = a[:len(a)-1]
				break
			}
		}
		// bcast the loss of routes through the channel
		for address, nodeInfo := range r.remoteNodeMap {
			if nodeInfo.Channel == channel {
				log.Print("removing: ", address)
				r.removeAddress(address)
				for _, ch := range r.channels {
					log.Print("sending reset packet")
					ch.Send(makeNetworkRouteSharePacket(r.address, address, 0))
				}
			} else {
				log.Print("keeping: ", address)
			}
		}
	}()
}

func (r *Router) removeAddress(remoteAddress AddressType) {
	// remove node and relay
	delete(r.remoteNodeMap, remoteAddress)
	for _, nodeLoadMap := range r.serviceLoadMap {
		delete(nodeLoadMap, remoteAddress)
	}
}

// AddChannel initializes
func (r *Router) AddChannel(channel Channel) {
	r.mutex.Lock()
	r.channels = append(r.channels, channel)
	r.mutex.Unlock()

	// define onPacket() to either handle link-state updates or route data
	go channel.Receive(func(packet *Packet) {
		// fmt.Printf("received packet dst:'%s' src:'%s' nxt:'%s' net:%d ctx:%d srv:%d data:%v\n",
		// 	packet.DestAddr, packet.SrcAddr, packet.NextAddr, packet.NetState, packet.ContextID, packet.ServiceID, packet.Data)

		switch packet.NetState {
		case NET_ROUTE:
			// neighbor is sharing it's routing table
			if remoteAddress, nextHop, cost, err := parseNetworkRouteSharePacket(packet); err == nil {
				// msg := "NET_ROUTE [%s] remoteAddress:%s, nextHop:%s, cost:%d\n"
				// log.Printf(msg, r.address, remoteAddress, nextHop, cost)
				r.mutex.Lock()
				defer r.mutex.Unlock()

				if cost == 0 {
					// remove the route
					if remoteAddress == r.address {
						go func() {
							// a short delay lets the message finish proagating
							//then this node pushes net state on all channels
							time.Sleep(100 * time.Millisecond)
							r.ShareNetState()
						}()
					} else if _, ok := r.remoteNodeMap[remoteAddress]; ok {
						r.removeAddress(remoteAddress)
						// relay update on other channels
						for _, ch := range r.channels {
							if ch != channel {
								ch.Send(packet)
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
			} else {
				log.Print("error parsing NET_ROUTE", err)
			}

		case NET_SERVICE:
			if address, serviceID, serviceLoad, err := parseNetworkServiceSharePacket(packet); err == nil {
				// fmt.Printf("NET_SERVICE node:%s, service:%d, load:%d\n", address, serviceID, serviceLoad)
				r.mutex.Lock()
				defer r.mutex.Unlock()
				if _, ok := r.serviceLoadMap[serviceID]; !ok {
					r.serviceLoadMap[serviceID] = NodeLoadMap{}
				}
				r.serviceLoadMap[serviceID][address] = NodeLoad{
					Load:     serviceLoad,
					LastSeen: time.Now().UTC(),
				}
				for _, c := range r.channels {
					if c != channel {
						c.Send(packet)
					}
				}
				if packetList, ok := r.serviceQueue[serviceID]; ok {
					if route, ok := r.remoteNodeMap[address]; ok {
						for _, p := range packetList {
							p.DestAddr = address
							p.NextAddr = route.NextHop
							channel.Send(p)
						}
					}
					delete(r.serviceQueue, serviceID)
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
		default:
			r.Send(packet)
		}
	}, r.RemoveChannel)
	channel.Send(makeNetQueryPacket())
}

// RegisterService binds a handler to a serviceID and shares the info with neighbors
func (r *Router) RegisterService(serviceID uint16, onPacket func(*Packet)) {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	r.serviceMap[serviceID] = onPacket
	go r.ShareNetState()
}

// UnregisterService releases the handler for a service
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

// ExportServiceTable composes a list of packets encoding the service table of this node
func (r *Router) ExportServiceTable() []*Packet {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	services := []*Packet{}
	for serviceID, _ := range r.serviceMap {
		var load uint16 // TODO measure load
		services = append(services, makeNetworkServiceSharePacket(r.address, serviceID, load))
	}
	for serviceID, loadMap := range r.serviceLoadMap {
		for address, load := range loadMap { // TODO sort by increasing load (first tx'd is lowest load)
			// TODO filter expired or unroutable entries
			services = append(services, makeNetworkServiceSharePacket(address, serviceID, load.Load))
		}
	}
	return services
}

// ShareNetState broadcasts the local routing table
func (r *Router) ShareNetState() {
	routes := r.ExportRouteTable()
	services := r.ExportServiceTable()
	r.mutex.Lock()
	defer r.mutex.Unlock()
	for _, channel := range r.channels {
		for _, routePacket := range routes {
			channel.Send(routePacket)
		}
		for _, servicePacket := range services {
			channel.Send(servicePacket)
		}
	}
}
