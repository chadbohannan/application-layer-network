import { Packet } from './packet.js'
import {
    makeNetQueryPacket,
    makeNetworkRouteSharePacket,
    parseNetworkRouteSharePacket,
    makeNetworkServiceSharePacket,
    parseNetworkServiceSharePacket
  } from './utils.js'
  
  export const NET_ROUTE = 0x01 // packet contains route entry
  export const NET_SERVICE = 0x02 // packet contains service entry
  export const NET_QUERY = 0x03 // packet is a request for content
  
  // An ALN is a set of router nodes connected by channels.
  
  // contextHandlerMap stores query response handers by contextID
  // type contextHandlerMap map[uint16]func(*Packet)
  
  // serviceHandlerMap maps local endpoint nodes to packet handlers
  // type serviceHandlerMap map[uint16]func(*Packet)
  
  export class RemoteNodeInfo {
    constructor (dst, nxt, ch, cost, lastSeen) {
      this.address = dst // target addres to communicate with
      this.nextHop = nxt // next routing service node for address
      this.channel = ch // specific channel that is hosting next hop
      this.cost = cost // cost of using this route, generally a hop count
      this.lastSeen = lastSeen // TODO: routes should decay over time
    }
  }
  
  // type RemoteNodeMap map[AddressType]*RemoteNodeInfo
  
  export class NodeCapacity {
    constructor (capacity, lastSeen) {
      this.capacity = capacity
      this.lastSeen = lastSeen
    }
  }
  // type NodeCapacityMap map[AddressType]NodeCapacity  // map[adress]NodeCapacity
  // type ServiceCapacityMap map[uint16]NodeCapacityMap // map[serviceID][address]NodeCapacity
  
  // Router manages a set of channels and packet handlers
  export class Router {
    constructor (address) {
      this.address = address
      this.channels = []
      this.serviceMap = new Map() // map[address]callback registered local node packet handlers
      this.contextMap = new Map() // map[uint16]func(packet)
      this.remoteNodeMap = new Map() // map[address]RemoteNodes[]
      this.serviceCapacityMap = new Map()// map[serviceID][address]NodeCapacity
      this.onNetStateChanged = () => { [console.log(`onNetStateChange ${this.remoteNodeMap.size} nodes`)]}
    }

    summary() {
      const res = []
      if (this.remoteNodeMap.size > 0) {
        const srvSum = this.serviceSummary()
        
        this.remoteNodeMap.forEach( (value, address) => {
          res.push({
            address:address,
            cost:value.cost,
            services: srvSum.filter(service => service.address == address),
          })
        })
      }
      return res
    }

    selectService (serviceID) {
      if (this.serviceMap.has(serviceID)) {
        return this.address
      }
  
      let maxCapacity = 0
      let remoteAddress = ''
      if (this.serviceCapacityMap.has(serviceID)) {
        const capacityMap = this.serviceCapacityMap.get(serviceID)
        capacityMap.forEach((nodeCapacity, address) => {
          if (maxCapacity < nodeCapacity.capacity) {
            maxCapacity = nodeCapacity.capacity
            remoteAddress = address
          }
        })
      }
      return remoteAddress
    }
  
    serviceAddresses (serviceID) {
      const addresses = [];
      if (this.serviceMap.has(serviceID)) {
        addresses.push(this.address)
      }
      if (this.serviceCapacityMap.has(serviceID)) {
        const capacityMap = this.serviceCapacityMap.get(serviceID)
        capacityMap.forEach((nodeCapacity, address) => {
          addresses.push(address)
        })
      }
      return addresses
    }

    // send() is the core routing function of Router. A packet is either expected
    //  locally by a registered Node, or on a multi-hop route to it's destination.
    send (packet) {
      if (!packet.src) {
        packet.src = this.address
      }
  
      // An ALN feature is to automatically send to any/all node(s) advertising the requested service
      if (!packet.dst && packet.srv) { // if service is set but destination is not
        const addresses = this.serviceAddresses(packet.srv)
        if (addresses.length > 0) {
          for (let i = 1; i < addresses.length; i++) {
            const pc = packet.copy ? packet.copy() : { ...packet }
            pc.dst = addresses[i]
            this.send(pc)
          }
          packet.dst = addresses[0]
          // Fall through to routing logic below
        } else {
          return 'no service providers found for: ' + packet.srv
        }
      }

      if (packet.dst === this.address) {
        // Check for context handler first (request-response pattern)
        if (packet.ctx && this.contextMap.has(packet.ctx)) {
          this.contextMap.get(packet.ctx)(packet)
        } else if (this.serviceMap.has(packet.srv)) {
          this.serviceMap.get(packet.srv)(packet)
        } else {
          return `no handler for packet - srv: ${packet.srv}, ctx: ${packet.ctx}`
        }
        return null
      }

      if (!packet.nxt || packet.nxt === this.address) {
        if (this.remoteNodeMap.has(packet.dst)) {
          const route = this.remoteNodeMap.get(packet.dst)
          packet.nxt = route.nextHop
          route.channel.send(packet)
          return null
        } else {
          return 'no route to destination: ' + packet.dst
        }
      }

      return 'packet is unroutable; no action taken'
    }
  
    // registerContextHandler returns a new contextID. Services must respond with the same
    // contextID to reach the correct response handler.
    registerContextHandler (packetHandler) {
      let ctxID = Math.floor(Math.random() * (2 << 15))
      while (ctxID === 0 || this.contextMap.has(ctxID)) {
        ctxID = Math.floor(Math.random() * (2 << 15))
      }
      this.contextMap.set(ctxID, packetHandler)
      return ctxID
    }
  
    // releaseContext frees memory associated with a context
    releaseContext (ctxID) {
      this.contextMap.delete(ctxID)
    }
  
    removeChannel (channel) {
      this.channels = this.channels.filter((ch) => ch !== channel)
    }
  
    onPacket (packet, channel) {
      switch (packet.net) {
        case NET_ROUTE: {
          // neighbor is sharing it's routing table
          const [remoteAddress, nextHop, cost, routeErr] = parseNetworkRouteSharePacket(packet)
          if (routeErr === null) {
            let remoteNode = this.remoteNodeMap.get(remoteAddress)
            if (!remoteNode) {
              remoteNode = new RemoteNodeInfo(remoteAddress, nextHop, channel, cost, Date.now())
              this.remoteNodeMap.set(remoteAddress, remoteNode)
            }
            var p
            if (cost === 0) {
              this.remoteNodeMap.delete(remoteAddress)
              this.serviceCapacityMap.forEach((capacityMap, serviceID) => {
                capacityMap.delete(remoteAddress)
              })
              p = makeNetworkRouteSharePacket(this.address, remoteAddress, 0)
            } else if (cost <= remoteNode.cost) {
              remoteNode.nextHop = nextHop
              remoteNode.channel = channel
              remoteNode.cost = cost
              p = makeNetworkRouteSharePacket(this.address, remoteAddress, cost + 1)
            }
            // relay update to other channels
            this.channels.forEach((ch) => (ch !== channel) ? ch.send(p) : {})
          } else {
            console.log(`error parsing NET_ROUTE: ${routeErr}`)
          }
        }
        this.onNetStateChanged()
        break
        case NET_SERVICE: {
          const [address, serviceID, serviceCapacity, serviceErr] = parseNetworkServiceSharePacket(packet)
          if (serviceErr === null) {
            let serviceChanged = false

            if (serviceCapacity === 0) {
              // Service removal (capacity = 0)
              if (this.serviceCapacityMap.has(serviceID)) {
                const capacityMap = this.serviceCapacityMap.get(serviceID)
                if (capacityMap.has(address)) {
                  capacityMap.delete(address)
                  serviceChanged = true
                }
              }
            } else {
              // Service addition/update
              if (!this.serviceCapacityMap.has(serviceID)) {
                this.serviceCapacityMap.set(serviceID, new Map())
              }
              this.serviceCapacityMap.get(serviceID).set(address, new NodeCapacity(serviceCapacity, Date.now()))
              serviceChanged = true
            }

            if (serviceChanged) {
              this.channels.forEach((ch) => (ch !== channel) ? ch.send(packet) : {})
              this.onNetStateChanged()
            }
          } else {
            console.log(`error parsing NET_SERVICE: ${serviceErr}`)
          }
        } break
        case NET_QUERY:
          this.exportRouteTable().forEach((p) => channel.send(p))
          this.exportServiceTable().forEach((p) => channel.send(p))
          break
        default:
          this.send(packet)
      }
    }
  
    // attach a new communication channel to the router
    addChannel (channel) {
      this.channels.push(channel)
      channel.onPacket = (packet) => {
        this.onPacket(packet, channel)
      }
      channel.onClose = () => { this.removeChannel(channel) }
      channel.send(makeNetQueryPacket())
    }
  
    // set or replace the packet handler for a serviceID
    registerService (serviceID, onPacket) {
      this.serviceMap.set(serviceID, onPacket)
      this.shareNetState() // push updated network service state
    }
  
    // stop handling service requests
    unregisterService (serviceID) {
      this.serviceMap.delete(serviceID)
    }
  
    // generate a list of packets that capture a snapshop of the routing tables
    exportRouteTable () {
      // one local route, with a cost of 1
      const dump = [makeNetworkRouteSharePacket(this.address, this.address, 1)]
  
      // for each remote route: our cost + 1
      this.remoteNodeMap.forEach((remoteNode, address) => {
        dump.push(makeNetworkRouteSharePacket(this.address, address, remoteNode.cost + 1))
      })
      return dump
    }
  
    // generate a list of packets that capture a snapshot of the service capacity table
    exportServiceTable () {
      const dump = []
      this.serviceMap.forEach((_, serviceID) => {
        dump.push(makeNetworkServiceSharePacket(this.address, serviceID, 1))
      })
      this.serviceCapacityMap.forEach((capacityMap, serviceID) => {
        capacityMap.forEach((nodeCapacity, address) => {
          // TODO sort by decreasing capacity (first tx'd is highest capacity)
          // TODO filter expired or unroutable entries
          dump.push(makeNetworkServiceSharePacket(address, serviceID, nodeCapacity.capacity))
        })
      })
      return dump
    }
  
    serviceSummary() {
      const dump = []
      this.serviceMap.forEach((_, serviceID) => {
        dump.push({address:this.address, service:serviceID, capacity: 1})
      })
      this.serviceCapacityMap.forEach((capacityMap, serviceID) => {
        capacityMap.forEach((nodeCapacity, address) => {
          dump.push({address:address, name:serviceID, capacity: nodeCapacity.capacity})
        })
      })
      return dump
    }

    shareNetState () {
      const routes = this.exportRouteTable()
      const services = this.exportServiceTable()
      this.channels.forEach((ch) => {
        routes.forEach((p) => ch.send(p))
        services.forEach((p) => ch.send(p))
      })
    }
  }
  
 