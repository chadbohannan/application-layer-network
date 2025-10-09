import { BinaryBuilder, BinaryParser } from './binary-utils.js'
import { Packet } from './packet.js'

export const NET_ROUTE = 0x01 // packet contains route entry
export const NET_SERVICE = 0x02 // packet contains service entry
export const NET_QUERY = 0x03 // packet is a request for content

export function makeNetQueryPacket () {
  const p = new Packet()
  p.net = NET_QUERY
  return p
}

export function makeNetworkRouteSharePacket (srcAddr, destAddr, cost) {
  const buf = new BinaryBuilder(destAddr.length + 3)
  buf.writeUint8(destAddr.length)
  buf.writeUTF8String(destAddr)
  buf.writeUint16(cost)
  const p = new Packet()
  p.net = NET_ROUTE
  p.src = srcAddr
  p.data = buf.toBinary()
  p.sz = p.data.length
  return p
}

// returns dest, next-hop, cost, err
export function parseNetworkRouteSharePacket (packet) {
  if (packet.net !== NET_ROUTE) {
    return ['', '', 0, 'parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE']
  }
  if (!packet.data) {
    return ['', '', 0, 'parseNetworkRouteSharePacket: packet.Data is empty']
  }
  const dataBuf = new BinaryParser(packet.data)
  const addrSize = dataBuf.readUint8()
  if (packet.data.length !== addrSize + 3) {
    return ['', '', 0, `parseNetworkRouteSharePacket: packet.data is ${packet.data.len}; expected ${addrSize + 3}`]
  }
  const dest = dataBuf.readBytes(addrSize).toUTF8()
  const cost = dataBuf.readUint16()
  const nextHop = packet.src
  return [dest, nextHop, cost, null]
}

export function makeNetworkServiceSharePacket (hostAddr, serviceID, serviceLoad) {
  const buf = new BinaryBuilder(hostAddr.length + serviceID.length + 4)
  buf.writeUint8(hostAddr.length)
  buf.writeUTF8String(hostAddr)

  buf.writeUint8(serviceID.length)
  buf.writeUTF8String(serviceID)

  buf.writeUint16(serviceLoad)
  const p = new Packet()
  p.net = NET_SERVICE
  p.data = buf.toBinary()
  p.sz = p.data.length
  return p
}

// returns address, serviceID, load, error
export function parseNetworkServiceSharePacket (packet) {
  if (packet.net !== NET_SERVICE) {
    return ['', 0, 0, 'parseNetworkServiceSharePacket: packet.NetState != NET_ROUTE']
  }
  const dataBuf = new BinaryParser(packet.data)
  const addrSize = dataBuf.readUint8()
  const addr = dataBuf.readBytes(addrSize).toUTF8()

  const serviceNameSize = dataBuf.readUint8()
  const serviceID = dataBuf.readBytes(serviceNameSize).toUTF8()

  const serviceLoad = dataBuf.readUint16()
  return [addr, serviceID, serviceLoad, null]
}
