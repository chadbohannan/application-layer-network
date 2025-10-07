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
type serviceHandlerMap map[string]func(*Packet)

// ServiceCapacityChangedHandler is called when a service capacity changes
type ServiceCapacityChangedHandler func(service string, capacity uint16, address AddressType)

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
type ServiceLoadMap map[string]NodeLoadMap // map[service][address]NodeLoad

// Router manages a set of channels and packet handlers
type Router struct {
	mutex                    sync.Mutex
	address                  AddressType
	channels                 []Channel
	serviceMap               serviceHandlerMap // map[address]callback registered local node packet handlers
	contextMap               contextHandlerMap
	remoteNodeMap            RemoteNodeMap // map[address][]RemoteNodes
	serviceLoadMap           ServiceLoadMap
	onServiceCapacityChanged ServiceCapacityChangedHandler
}

// NewRouter instantiates an applications ELP Router
func NewRouter(address AddressType) *Router {
	return &Router{
		address:        address,
		channels:       make([]Channel, 0),
		serviceMap:     make(serviceHandlerMap),
		contextMap:     make(contextHandlerMap),
		remoteNodeMap:  make(RemoteNodeMap),
		serviceLoadMap: make(ServiceLoadMap),
	}
}

func (r *Router) Address() AddressType {
	return r.address
}

// SetOnServiceCapacityChangedHandler sets the callback for service capacity changes
func (r *Router) SetOnServiceCapacityChangedHandler(handler ServiceCapacityChangedHandler) {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	r.onServiceCapacityChanged = handler
}

// notifyServiceCapacityChanged calls the handler if set (assumes mutex is already locked)
func (r *Router) notifyServiceCapacityChanged(service string, capacity uint16, address AddressType) {
	if r.onServiceCapacityChanged != nil {
		go r.onServiceCapacityChanged(service, capacity, address)
	}
}

// routeAndSend handles the actual routing and sending of a packet.
// This function assumes the router's mutex is already locked.
func (r *Router) routeAndSend(p *Packet) error {
	// log.Printf("DEBUG: routeAndSend called with DestAddr='%s', SrcAddr='%s', Service='%s', ContextID=%d", p.DestAddr, p.SrcAddr, p.Service, p.ContextID)
	if p.DestAddr == r.address {
		if onPacket, ok := r.serviceMap[p.Service]; ok {
			onPacket(p)
		} else if onPacket, ok = r.contextMap[p.ContextID]; ok {
			onPacket(p)
		} else {
			return fmt.Errorf("service '%s' not registered", p.Service)
		}
	} else if p.NextAddr == r.address || len(p.NextAddr) == 0 {
		if route, ok := r.remoteNodeMap[p.DestAddr]; ok {
			p.NextAddr = route.NextHop
			route.Channel.Send(p)
		} else {
			return fmt.Errorf("failed to find route for %s", p.DestAddr)
		}
	} else {
		return fmt.Errorf("packet is unroutable; no action taken")
	}
	return nil
}

// ServiceAddresses returns all the addresses that advertise a specific service
func (r *Router) ServiceAddresses(service string) []AddressType {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	addresses := []AddressType{}
	if _, ok := r.serviceMap[service]; ok {
		addresses = append(addresses, r.address)
	}
	if loadMap, ok := r.serviceLoadMap[service]; ok {
		for address, _ := range loadMap {
			addresses = append(addresses, address)
		}
	}
	return addresses
}

// SelectService returns the address of the service with the lowest load, or zero
func (r *Router) SelectService(service string) AddressType {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	if _, ok := r.serviceMap[service]; ok {
		return r.address
	}
	var minLoad uint16
	var remoteAddress AddressType
	if loadMap, ok := r.serviceLoadMap[service]; ok {
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
//
//	locally by a registered Node, or on a multi-hop route to it's destination.
func (r *Router) Send(p *Packet) error {
	if len(p.SrcAddr) == 0 {
		p.SrcAddr = r.address
	}

	if len(p.DestAddr) == 0 && len(p.Service) != 0 {
		// service multicast; send to all service instances

		r.mutex.Lock()
		// First, handle local delivery if the router itself provides the service
		if onPacket, ok := r.serviceMap[p.Service]; ok {
			if pc, err := p.Copy(); err == nil {
				pc.DestAddr = r.address // Ensure local delivery
				onPacket(pc)
			}
		}
		r.mutex.Unlock()

		serviceAddresses := r.ServiceAddresses(p.Service)
		for _, destAddr := range serviceAddresses {
			if destAddr != r.address { // Avoid sending to self again if already handled locally
				if pc, err := p.Copy(); err == nil {
					pc.DestAddr = destAddr
					go func(packet *Packet) {
						r.mutex.Lock()
						defer r.mutex.Unlock()
						if err := r.routeAndSend(packet); err != nil {
							log.Printf("error routing multicast packet to %s: %v", packet.DestAddr, err)
						}
					}(pc)
				}
			}
		}
		return nil // Multicast handled, no further processing for this packet in this Send call
	}

	r.mutex.Lock()
	defer r.mutex.Unlock()

	return r.routeAndSend(p)
}

// RegisterContextHandler returns a contextID. Services must respond with the same
// contextID to reach the correct response handler.
func (r *Router) RegisterContextHandler(packetHandler func(*Packet)) uint16 {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	ctxID := uint16(rand.Intn(2 << 16))
	for _, ok := r.contextMap[ctxID]; ok; {
		ctxID = uint16(rand.Intn(2 << 16))
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
	// log.Printf("router:RemoveChannel")
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
			r.removeAddress(address)
			for _, ch := range r.channels {
				ch.Send(makeNetworkRouteSharePacket(r.address, address, 0))
			}
		}
	}
}

func (r *Router) removeAddress(remoteAddress AddressType) {
	// remove node and relay
	delete(r.remoteNodeMap, remoteAddress)
	// Notify about service removals before deleting
	for service, nodeLoadMap := range r.serviceLoadMap {
		if _, exists := nodeLoadMap[remoteAddress]; exists {
			r.notifyServiceCapacityChanged(service, 0, remoteAddress)
			delete(nodeLoadMap, remoteAddress)
		}
	}
}

// AddChannel initializes
func (r *Router) AddChannel(channel Channel) {
	r.mutex.Lock()
	r.channels = append(r.channels, channel)
	r.mutex.Unlock()

	// define onPacket() to either handle link-state updates or route data
	go channel.Receive(func(packet *Packet) bool {
		// log.Printf("received packet srv:'%s' dst:'%s' src:'%s' nxt:'%s' net:%d ctx:%d data:%v\n",
		// 	packet.Service, packet.DestAddr, packet.SrcAddr, packet.NextAddr, packet.NetState, packet.ContextID, packet.Data)

		// log.Printf("DEBUG: packet.NetState=%d, SrcAddr='%s', DestAddr='%s'", packet.NetState, packet.SrcAddr, packet.DestAddr)
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
				} else if remoteAddress != r.address {
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
			if address, service, serviceLoad, err := parseNetworkServiceSharePacket(packet); err == nil {
				if address == r.address {
					break
				}
				// log.Printf("NET_SERVICE node:'%s', service:'%s', load:%d\n", address, service, serviceLoad)
				r.mutex.Lock()
				serviceChanged := false

				if _, ok := r.serviceLoadMap[service]; !ok {
					r.serviceLoadMap[service] = NodeLoadMap{}
				}

				if serviceLoad == 0 {
					// Service removal
					if _, ok := r.serviceLoadMap[service][address]; ok {
						delete(r.serviceLoadMap[service], address)
						serviceChanged = true
					}
				} else {
					// Service addition or update
					if nl, ok := r.serviceLoadMap[service][address]; ok {
						if nl.Load == serviceLoad {
							r.mutex.Unlock()
							return true // drop packet to avoid propagation loops
						}
						serviceChanged = true
					} else {
						serviceChanged = true // new service
					}
					r.serviceLoadMap[service][address] = NodeLoad{
						Load:     serviceLoad,
						LastSeen: time.Now().UTC(),
					}
				}

				if serviceChanged {
					r.notifyServiceCapacityChanged(service, serviceLoad, address)
				}

				r.mutex.Unlock()

				// Relay to other channels
				for _, c := range r.channels {
					if c != channel {
						c.Send(packet)
					}
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
		case NET_ERROR:
			log.Printf("NET_ERROR 255 %s", string(packet.Data))
		default:
			// For regular data packets, directly process or forward
			r.mutex.Lock()

			// Auto-learn route to packet source for direct TCP connections
			if len(packet.SrcAddr) > 0 && packet.SrcAddr != r.address {
				if _, exists := r.remoteNodeMap[packet.SrcAddr]; !exists {
					r.remoteNodeMap[packet.SrcAddr] = &RemoteNodeInfo{
						Address:  packet.SrcAddr,
						NextHop:  packet.SrcAddr, // Direct connection
						Channel:  channel,
						Cost:     1, // Direct connection cost
						LastSeen: time.Now().UTC(),
					}
					log.Printf("Auto-learned route to %s via channel", packet.SrcAddr)
				}
			}

			// Assuming the packet has arrived at its final destination or needs to be forwarded
			if packet.DestAddr == r.address {
				// Copy handler reference and release mutex before calling to avoid deadlock
				var handler func(*Packet)
				var found bool
				if handler, found = r.serviceMap[packet.Service]; found {
					// Found service handler
				} else if handler, found = r.contextMap[packet.ContextID]; found {
					// Found context handler
				} else {
					log.Printf("received unhandled packet for service '%s' or context '%d'", packet.Service, packet.ContextID)
				}

				r.mutex.Unlock()

				if found && handler != nil {
					handler(packet) // Call handler WITHOUT holding mutex
				}
			} else if route, ok := r.remoteNodeMap[packet.DestAddr]; ok {
				packet.NextAddr = route.NextHop
				route.Channel.Send(packet)
				r.mutex.Unlock()
			} else {
				log.Printf("packet is unroutable; no action taken for dest %s", packet.DestAddr)
				r.mutex.Unlock()
			}
		}
		return true
	})
	channel.OnClose(func(c Channel) {
		r.RemoveChannel(c)
	})
	channel.Send(makeNetQueryPacket())
}

// RegisterService binds a handler to a service and shares the info with neighbors
func (r *Router) RegisterService(service string, onPacket func(*Packet)) {
	r.mutex.Lock()
	r.serviceMap[service] = onPacket
	r.notifyServiceCapacityChanged(service, 1, r.address) // Notify about local service with capacity 1
	r.mutex.Unlock()
	go r.ShareNetState()
}

// UnregisterService releases the handler for a service
func (r *Router) UnregisterService(service string) {
	r.mutex.Lock()
	defer r.mutex.Unlock()

	if _, exists := r.serviceMap[service]; !exists {
		return // Service not registered
	}

	delete(r.serviceMap, service)
	r.notifyServiceCapacityChanged(service, 0, r.address) // Notify about service removal

	// Send removal notification to all channels
	removalPacket := makeNetworkServiceSharePacket(r.address, service, 0)
	for _, channel := range r.channels {
		channel.Send(removalPacket)
	}
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

func (r *Router) NumChannels() int {
	return len(r.channels)
}

// HasRoute returns true if the router has a route to the given address
func (r *Router) HasRoute(address AddressType) bool {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	_, ok := r.remoteNodeMap[address]
	return ok
}

// HasService returns true if the router has a service registered locally or remotely
func (r *Router) HasService(service string) bool {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	if _, ok := r.serviceMap[service]; ok {
		return true
	}
	if loadMap, ok := r.serviceLoadMap[service]; ok {
		return len(loadMap) > 0
	}
	return false
}

// ExportServiceTable composes a list of packets encoding the service table of this node
func (r *Router) ExportServiceTable() []*Packet {
	r.mutex.Lock()
	defer r.mutex.Unlock()
	services := []*Packet{}
	for service := range r.serviceMap {
		var load uint16 = 1 // TODO measure load
		services = append(services, makeNetworkServiceSharePacket(r.address, service, load))
	}
	for service, loadMap := range r.serviceLoadMap {
		for address, load := range loadMap { // TODO sort by increasing load (first tx'd is lowest load)
			// TODO filter expired or unroutable entries
			// log.Printf("sharing remote service: %s at %s", service, address)
			services = append(services, makeNetworkServiceSharePacket(address, service, load.Load))
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
