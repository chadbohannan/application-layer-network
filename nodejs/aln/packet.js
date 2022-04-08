const ByteBuffer = require('bytebuffer')
const CF_NETSTATE = 0x0400
const CF_SERVICEID = 0x0200
const CF_SRCADDR = 0x0100
const CF_DESTADDR = 0x0080
const CF_NEXTADDR = 0x0040
const CF_SEQNUM = 0x0020
const CF_ACKBLOCK = 0x0010
const CF_CONTEXTID = 0X0008
const CF_DATATYPE = 0X0004
const CF_DATA = 0x0002
const CF_CRC = 0x0001

class Packet {
  constructor (content) {
    this.cf = 0
    this.net = 0
    this.srv = 0
    this.src = ''
    this.dst = ''
    this.nxt = ''
    this.seq = 0
    this.ack = 0
    this.ctx = 0
    this.typ = 0
    this.sz = 0
    this.data = null
    this.crc = null
    if (content) {
      const buf = ByteBuffer.fromBinary(content)
      this.cf = buf.readUint16()
      if (this.cf & CF_NETSTATE) this.net = buf.readUint8()
      if (this.cf & CF_SERVICEID) this.srv = buf.readUint16()
      if (this.cf & CF_SRCADDR) this.src = buf.readBytes(buf.readUint8()).toUTF8()
      if (this.cf & CF_DESTADDR) this.dst = buf.readBytes(buf.readUint8()).toUTF8()
      if (this.cf & CF_NEXTADDR) this.nxt = buf.readBytes(buf.readUint8()).toUTF8()
      if (this.cf & CF_SEQNUM) this.seq = buf.readUint16()
      if (this.cf & CF_ACKBLOCK) this.ack = buf.readUint32()
      if (this.cf & CF_CONTEXTID) this.ctx = buf.readUint16()
      if (this.typ & CF_DATATYPE) this.typ = buf.readUint8()
      if (this.cf & CF_DATA) {
        this.sz = buf.readUint8()
        this.data = buf.readBytes(this.sz).toBinary(0, this.sz)
      }
      if (this.cf & CF_CRC) this.crc = buf.readUint32() // TODO verify CRC
    }
  }

  toJson () {
    return JSON.stringify({ cf: this.cf, net: this.net, srv: this.srv, src: this.src, dst: this.dst, nxt: this.nxt, data_sz: this.sz })
  }

  toBinary () {
    const buf = new ByteBuffer(4096)
    let cf = 0x0000
    buf.writeUint16(cf)
    if (this.net) (cf |= CF_NETSTATE) && buf.writeByte(this.net)
    if (this.srv) (cf |= CF_SERVICEID) && buf.writeByte(this.srv)
    if (this.src) (cf |= CF_SRCADDR) && buf.writeByte(this.src.length) && buf.writeUTF8String(this.src)
    if (this.dst) (cf |= CF_DESTADDR) && buf.writeByte(this.dst.length) && buf.writeUTF8String(this.dst)
    if (this.nxt) (cf |= CF_NEXTADDR) && buf.writeByte(this.nxt.length) && buf.writeUTF8String(this.nxt)
    if (this.seq) (cf |= CF_SEQNUM) && buf.writeUint16(this.seq)
    if (this.ack) (cf |= CF_ACKBLOCK) && buf.writeUint32(this.ack)
    if (this.ctx) (cf |= CF_CONTEXTID) && buf.writeUint16(this.ctx)
    if (this.typ) (cf |= CF_DATATYPE) && buf.writeUint8(this.typ)
    if (this.data) {
      cf |= CF_DATA
      buf.writeUint16(this.data.length)
      for (let i = 0; i < this.data.length; i++) {
        buf.writeByte(this.data[i])
      }
    }
    const offset = buf.offset
    this.cf = cfHamEncode(cf)
    buf.reset().writeUint16(this.cf)
    const ret = buf.reset().toBinary(0, offset)
    return ret
  }
}

function intXOR (n) {
  let cnt = 0
  while (n) { // This loop will only execute the number times equal to the number of ones. */
    cnt ^= 0x1
    n &= (n - 0x1)
  }
  return cnt
}

function cfHamEncode (value) {
  return (value & 0x07FF) |
    (intXOR(value & 0x071D) << 12) |
    (intXOR(value & 0x04DB) << 13) |
    (intXOR(value & 0x01B7) << 14) |
    (intXOR(value & 0x026F) << 15)
}

// function cfHamDecode (value) {
//   const hamDecodMap = {
//     0x0F: 0x0001,
//     0x07: 0x0002,
//     0x0B: 0x0004,
//     0x0D: 0x0008,
//     0x0E: 0x0010,
//     0x03: 0x0020,
//     0x05: 0x0040,
//     0x06: 0x0080,
//     0x0A: 0x0100,
//     0x09: 0x0200,
//     0x0C: 0x0400
//   }
//   const err = intXOR(value & 0x826F) |
//       (intXOR(value & 0x41B7) << 1) |
//       (intXOR(value & 0x24DB) << 2) |
//       (intXOR(value & 0x171D) << 3) &
//       0xFF

//   value ^= hamDecodMap.get(err, 0)
//   return value
// }

module.exports.Packet = Packet
