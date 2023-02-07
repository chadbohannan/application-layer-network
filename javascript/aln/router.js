import { Packet } from './packet'
import {
    makeNetQueryPacket,
    makeNetworkRouteSharePacket,
    parseNetworkRouteSharePacket,
    makeNetworkServiceSharePacket,
    parseNetworkServiceSharePacket
  } from './utils'
  
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
  
  export class NodeLoad {
    constructor (load, lastSeen) {
      this.load = load
      this.lastSeen = lastSeen
    }
  }
  // type NodeLoadMap map[AddressType]NodeLoad  // map[adress]NodeLoad
  // type ServiceLoadMap map[uint16]NodeLoadMap // map[serviceID][address]NodeLoad
  
  // Router manages a set of channels and packet handlers
  export class Router {
    constructor (address) {
      this.address = address
      this.channels = []
      this.serviceMap = new Map() // map[address]callback registered local node packet handlers
      this.contextMap = new Map() // map[uint16]func(packet)
      this.remoteNodeMap = new Map() // map[address]RemoteNodes[]
      this.serviceLoadMap = new Map()// map[serviceID][address]NodeLoad
      this.serviceQueue = new Map() // map[uint16]packet[] packets waiting for services during warmup
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
  
      let minLoad = 0
      let remoteAddress = ''
      if (this.serviceLoadMap.has(serviceID)) {
        const loadMap = this.serviceLoadMap.get(serviceID)
        loadMap.forEach((nodeLoad, address) => {
          if (minLoad === 0 || minLoad > nodeLoad.load) {
            remoteAddress = address
            minLoad = nodeLoad.load
          }
        })
      }
      return remoteAddress
    }
  
    // send() is the core routing function of Router. A packet is either expected
    //  locally by a registered Node, or on a multi-hop route to it's destination.
    send (packet) {
      if (!packet.src) {
        packet.src = this.address
      }
  
      // An ALN feature is to automatically send to a network node advertising the requested service
      if (!packet.dst && packet.srv) { // if destination is not set but a service is
        // use the service load cache to select a service host node address
        packet.dst = this.selectService(packet.srv)
        if (!packet.dst) { // didn't find a service
          // best effort local caching to give network discovery a chance
          // TODO cache expiration+cleanup
          let q = []
          if (this.serviceQueue.has(packet.srv)) {
            q = this.serviceQueue.get(packet.srv)
          }
          q.push(packet)
          this.serviceQueue.set(packet.srv, q)
          return 'no route for service, packet queued'
        }
      }
      if (packet.dst === this.address) {
        if (this.serviceMap.has(packet.srv)) {
          this.serviceMap.get(packet.srv)(packet)
        } else if (this.contextMap.has(packet.srv)) {
          this.contextMap.get(packet.srv)(packet)
        } else {
          return `service ${packet.srv} not registered`
        }
      } else if (!packet.nxt || packet.nxt === this.address) {
        if (this.remoteNodeMap.has(packet.dst)) {
          const route = this.remoteNodeMap.get(packet.dst)
          packet.nxt = route.nextHop
          route.channel.send(packet)
        }
      } else {
        return 'packet is unroutable; no action taken'
      }
      return null
    }
  
    // registerContextHandler returns a new contextID. Services must respond with the same
    // contextID to reach the correct response handler.
    registerContextHandler (packetHandler) {
      let ctxID = Math.floor(Math.random() * 2 << 15)
      while (this.contextMap.has(ctxID)) {
        ctxID = Math.floor(Math.random() * 2 << 15)
      }
      this.contextMap[ctxID] = packetHandler
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
              this.serviceLoadMap.forEach((loadMap, serviceID) => {
                loadMap.delete(remoteAddress)
              })
              p = makeNetworkRouteSharePacket(this.address, remoteAddress, 0)
            } else if (cost <= remoteNode.cost) {
              remoteNode.NextHop = nextHop
              remoteNode.Channel = channel
              remoteNode.Cost = cost
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
          const [address, serviceID, serviceLoad, serviceErr] = parseNetworkServiceSharePacket(packet)
          if (serviceErr === null) {
            if (!this.serviceLoadMap.has(serviceID)) {
              this.serviceLoadMap.set(serviceID, new Map())
            }
            this.serviceLoadMap.get(serviceID).set(address, new NodeLoad(serviceLoad, Date.now()))
            this.channels.forEach((ch) => (ch !== channel) ? ch.send(packet) : {})
            if (this.serviceQueue.has(serviceID)) {
              const packetList = this.serviceQueue.get(serviceID)
              if (this.remoteNodeMap.has(address)) {
                const route = this.remoteNodeMap.get(address)
                packetList.forEach((p) => {
                  p.dst = address
                  p.nxt = route.nextHop
                  channel.send(p)
                })
                this.serviceQueue.delete(serviceID)
              }
            }
            this.onNetStateChanged()
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
  
    // generate a list of packets that capture a snapshot of the service load table
    exportServiceTable () {
      const dump = []
      this.serviceMap.forEach((_, serviceID) => {
        dump.push(makeNetworkServiceSharePacket(this.address, serviceID, 0))
      })
      this.serviceLoadMap.forEach((loadMap, serviceID) => {
        loadMap.forEach((nodeLoad, address) => {
          // TODO sort by increasing load (first tx'd is lowest load)
          // TODO filter expired or unroutable entries
          dump.push(makeNetworkServiceSharePacket(address, serviceID, nodeLoad.load))
        })
      })
      return dump
    }
  
    serviceSummary() {
      const dump = []
      this.serviceMap.forEach((_, serviceID) => {
        dump.push({address:this.address, service:serviceID, load: 0})
      })
      this.serviceLoadMap.forEach((loadMap, serviceID) => {
        loadMap.forEach((nodeLoad, address) => {
          dump.push({address:address, name:serviceID, load: nodeLoad.load})
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
  
 