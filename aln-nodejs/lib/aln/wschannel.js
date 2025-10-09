const ByteBuffer = require('bytebuffer')
const { Packet } = require('./packet')

class WebSocketChannel {
  constructor (ws) {
    this.ws = ws
    this.onPacket = () => { console.log('WebSocketChannel::onPacket; default') }
    this.onClose = () => { console.log('WebSocketChannel::onClose; default') }

    ws.on('message', (data) => {
      console.log('ws received: %s', data)
      const obj = JSON.parse(data)
      const packet = new Packet()
      if (obj.cf) packet.cf = obj.cf
      if (obj.net) packet.net = obj.net
      if (obj.srv) packet.srv = obj.srv
      if (obj.src) packet.src = obj.src
      if (obj.dst) packet.dst = obj.dst
      if (obj.nxt) packet.nxt = obj.nxt
      if (obj.seq) packet.seq = obj.seq
      if (obj.ack) packet.ack = obj.ack
      if (obj.ctx) packet.ctx = obj.ctx
      if (obj.typ) packet.typ = obj.typ
      if (obj.data) packet.data = ByteBuffer.fromBase64(obj.data).toBinary()
      this.onPacket(packet) // Fixed: use parsed packet, not raw data
    })

    ws.on('close', () => {
      console.log('WebSocket connection closed')
      this.onClose()
    })

    ws.on('error', (error) => {
      console.log('WebSocket error:', error.message)
      // Error usually leads to close, but call onClose just in case
      this.onClose()
    })
  }

  send (packet) {
    this.ws.send(packet.toJson())
  }
}

module.exports.WebSocketChannel = WebSocketChannel
