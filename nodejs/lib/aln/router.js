const {
  makeNetQueryPacket,
  makeNetworkRouteSharePacket,
  parseNetworkRouteSharePacket,
  makeNetworkServiceSharePacket,
  parseNetworkServiceSharePacket
} = require('./utils')

const NET_ROUTE = 0x01 // packet contains route entry
const NET_SERVICE = 0x02 // packet contains service entry
const NET_QUERY = 0x03 // packet is a request for content

// An ALN is a set of router nodes connected by channels.

// contextHandlerMap stores query response handers by contextID
// type contextHandlerMap map[uint16]func(*Packet)

// serviceHandlerMap maps local endpoint nodes to packet handlers
// type serviceHandlerMap map[uint16]func(*Packet)

class RemoteNodeInfo {
  constructor (dst, nxt, ch, cost, lastSeen) {
    this.address = dst // target addres to communicate with
    this.nextHop = nxt // next routing service node for address
    this.channel = ch // specific channel that is hosting next hop
    this.cost = cost // cost of using this route, generally a hop count
    this.lastSeen = lastSeen // TODO: routes should decay over time
  }
}

// type RemoteNodeMap map[AddressType]*RemoteNodeInfo

class NodeCapacity {
  constructor (capacity, lastSeen) {
    this.capacity = capacity
    this.lastSeen = lastSeen
  }
}
// type NodeCapacityMap map[AddressType]NodeCapacity  // map[adress]NodeCapacity
// type ServiceCapacityMap map[uint16]NodeCapacityMap // map[serviceID][address]NodeCapacity

// Router manages a set of channels and packet handlers
class Router {
  constructor (address) {
    this.address = address
    this.channels = []
    this.serviceMap = new Map() // map[address]callback registered local node packet handlers
    this.contextMap = new Map() // map[uint16]func(packet)
    this.remoteNodeMap = new Map() // map[address]RemoteNodes[]
    this.serviceCapacityMap = new Map()// map[serviceID][address]NodeCapacity
    this.onServiceCapacityChanged = null // callback for service capacity changes
  }

  // setOnServiceCapacityChanged registers a callback for service capacity changes
  // callback signature: function(service, capacity, address)
  setOnServiceCapacityChanged (callback) {
    this.onServiceCapacityChanged = callback
  }

  serviceAddresses (serviceID) {
    const addresses = []
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

    // An ALN feature is to automatically send to a network node advertising the requested service
    if (!packet.dst && packet.srv) { // if destination is not set but a service is
      const addresses = this.serviceAddresses(packet.srv)
      if (addresses.length > 0) {
        for (let i = 1; i < addresses.length; i++) {
          const pc = packet.copy()
          pc.dst = addresses[i]
          this.send(pc)
        }
        packet.dst = addresses[0]
      }
      if (!packet.dst) { // didn't find a service
        return 'no service of type ' + packet.srv + ' yet discovered'
      }
    }
    if (packet.dst === this.address) {
      if (this.serviceMap.has(packet.srv)) {
        this.serviceMap.get(packet.srv)(packet)
      } else if (this.contextMap.has(packet.ctx)) {
        this.contextMap.get(packet.ctx)(packet)
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
    this.contextMap.set(ctxID, packetHandler)
    return ctxID
  }

  // releaseContext frees memory associated with a context
  releaseContext (ctxID) {
    this.contextMap.delete(ctxID)
  }

  removeChannel (channel) {
    this.channels = this.channels.filter((ch) => ch !== channel)
    this.remoteNodeMap.forEach((nodeInfo, address) => {
      if (nodeInfo.channel === channel) {
        this.remoteNodeMap.delete(address)
        this.serviceCapacityMap.forEach((capacityMap) => capacityMap.delete(address))
        const src = this.address
        this.channels.forEach((ch) => { ch.send(makeNetworkRouteSharePacket(src, address, 0)) })
      }
    })
  }

  onPacket (packet, channel) {
    switch (packet.net) {
      case NET_ROUTE: {
        // neighbor is sharing it's routing table
        const [remoteAddress, nextHop, cost, routeErr] = parseNetworkRouteSharePacket(packet)
        if (routeErr === null) {
          if (cost === 0) { // route is being reset on a channel closing
            if (remoteAddress === this.address) {
              // readvertize self
              const _this = this
              setTimeout(() => { _this.shareNetState() }, 100)
            } else {
              // remove route
              this.remoteNodeMap.delete(remoteAddress)
              this.serviceCapacityMap.forEach((capacityMap) => capacityMap.delete(remoteAddress))
              this.channels.forEach((ch) => {
                if (ch !== channel) ch.send(packet)
              })
            }
          } else {
            // add new route
            let remoteNode = this.remoteNodeMap.get(remoteAddress)
            if (!remoteNode) {
              remoteNode = new RemoteNodeInfo(remoteAddress, nextHop, channel, cost, Date.now())
              this.remoteNodeMap.set(remoteAddress, remoteNode)
            }
            if (remoteNode.cost === 0) {
              this.remoteNodeMap.delete(remoteAddress)
            } else if (cost <= remoteNode.cost) {
              remoteNode.nextHop = nextHop
              remoteNode.channel = channel
              remoteNode.cost = cost
              // relay update to other channels
              const p = makeNetworkRouteSharePacket(this.address, remoteAddress, cost + 1)
              this.channels.forEach((ch) => (ch !== channel) ? ch.send(p) : {})
            }
          }
        } else {
          console.log(`error parsing NET_ROUTE: ${routeErr}`)
        }
      } break
      case NET_SERVICE: {
        const [address, serviceID, serviceCapacity, serviceErr] = parseNetworkServiceSharePacket(packet)
        if (address === this.address) break
        if (serviceErr === null) {
          console.log(`NET_SERVICE node:${address}, service:${serviceID}, capacity:${serviceCapacity}`)

          let serviceChanged = false

          // Check if this is a new service or capacity changed
          if (!this.serviceCapacityMap.has(serviceID)) {
            this.serviceCapacityMap.set(serviceID, new Map())
          }

          const capacityMap = this.serviceCapacityMap.get(serviceID)

          if (serviceCapacity === 0) {
            // Service removal
            if (capacityMap.has(address)) {
              capacityMap.delete(address)
              serviceChanged = true
            }
          } else {
            // Service addition or update
            const existingCapacity = capacityMap.get(address)
            if (!existingCapacity || existingCapacity.capacity !== serviceCapacity) {
              capacityMap.set(address, new NodeCapacity(serviceCapacity, Date.now()))
              serviceChanged = true
            }
          }

          // Notify callback if service changed
          if (serviceChanged && this.onServiceCapacityChanged) {
            this.onServiceCapacityChanged(serviceID, serviceCapacity, address)
          }

          this.channels.forEach((ch) => (ch !== channel) ? ch.send(packet) : {})
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

  shareNetState () {
    const routes = this.exportRouteTable()
    const services = this.exportServiceTable()
    this.channels.forEach((ch) => {
      routes.forEach((p) => ch.send(p))
      services.forEach((p) => ch.send(p))
    })
  }
}

module.exports = Router
