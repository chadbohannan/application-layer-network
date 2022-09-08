const ByteBuffer = require('bytebuffer')
const { Packet } = require('./packet')

const NET_ROUTE = 0x01 // packet contains route entry
const NET_SERVICE = 0x02 // packet contains service entry
const NET_QUERY = 0x03 // packet is a request for content

function makeNetQueryPacket () {
  const p = new Packet()
  p.net = NET_QUERY
  return p
}

function makeNetworkRouteSharePacket (srcAddr, destAddr, cost) {
  const buf = new ByteBuffer(destAddr.length + 3)
  buf.writeUint8(destAddr.length)
  buf.writeUTF8String(destAddr)
  buf.writeUint16(cost)
  const p = new Packet()
  p.net = NET_ROUTE
  p.src = srcAddr
  p.data = buf.reset().toBinary()
  p.sz = p.data.length
  return p
}

// returns dest, next-hop, cost, err
function parseNetworkRouteSharePacket (packet) {
  if (packet.net !== NET_ROUTE) {
    return ['', '', 0, 'parseNetworkRouteSharePacket: packet.NetState != NET_ROUTE']
  }
  if (!packet.data) {
    return ['', '', 0, 'parseNetworkRouteSharePacket: packet.Data is empty']
  }
  const dataBuf = ByteBuffer.fromBinary(packet.data)
  const addrSize = dataBuf.readUint8()
  if (packet.data.length !== addrSize + 3) {
    return ['', '', 0, `parseNetworkRouteSharePacket: packet.data is ${packet.data.len}; expected ${addrSize + 3}`]
  }
  const dest = dataBuf.readBytes(addrSize).toUTF8()
  const cost = dataBuf.readUint16()
  const nextHop = packet.src
  return [dest, nextHop, cost, null]
}

function makeNetworkServiceSharePacket (hostAddr, serviceID, serviceLoad) {
  const buf = new ByteBuffer(hostAddr.length + serviceID.length + 4)
  buf.writeUint8(hostAddr.length)
  buf.writeUTF8String(hostAddr)
  buf.writeUint8(serviceID.length)
  buf.writeUTF8String(serviceID)
  buf.writeUint16(serviceLoad)
  const p = new Packet()
  p.net = NET_SERVICE
  p.data = buf.reset().toBinary()
  p.sz = p.data.length
  return p
}

// returns address, serviceID, load, error
function parseNetworkServiceSharePacket (packet) {
  const fname = 'parseNetworkServiceSharePacket'
  if (packet.net !== NET_SERVICE) {
    return ['', 0, 0, fname + ': packet.NetState != NET_ROUTE']
  }
  const dataBuf = ByteBuffer.fromBinary(packet.data)
  const addrSize = dataBuf.readUint8()

  const addr = dataBuf.readBytes(addrSize).toUTF8()
  const serviceIDsize = dataBuf.readUint8()
  const serviceID = dataBuf.readBytes(serviceIDsize).toUTF8()
  const serviceLoad = dataBuf.readUint16()
  return [addr, serviceID, serviceLoad, null]
}

module.exports.makeNetQueryPacket = makeNetQueryPacket
module.exports.makeNetworkRouteSharePacket = makeNetworkRouteSharePacket
module.exports.parseNetworkRouteSharePacket = parseNetworkRouteSharePacket
module.exports.makeNetworkServiceSharePacket = makeNetworkServiceSharePacket
module.exports.parseNetworkServiceSharePacket = parseNetworkServiceSharePacket
