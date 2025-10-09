import { base64ToBinary } from './binary-utils.js'
import { Packet } from './packet.js'

export class WebSocketChannel {
  constructor (ws) {
    this.ws = ws
    this.onPacket = () => { console.log("WebSocketChannel::onPacket; default")}
    this.onClose = () => { console.log("WebSocketChannel::onClose; default")}

    ws.onmessage = (event) => {
      const obj = JSON.parse(event.data)
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
      if (obj.data) packet.data = base64ToBinary(obj.data)
      this.onPacket(packet)
    };
    ws.onerror = (event) => {
        console.log(event);
    };
    
    ws.onclose = (event) => {
      this.onClose()
    };
  }

  close() {
    this.onClose()
    this.ws.close()
  }

  send(packet) {
    // Handle both Packet instances and plain objects
    if (packet.toJson) {
      this.ws.send(packet.toJson())
    } else {
      // Convert plain object to JSON
      const pkt = new Packet()
      if (packet.cf !== undefined) pkt.cf = packet.cf
      if (packet.net !== undefined) pkt.net = packet.net
      if (packet.srv) pkt.srv = packet.srv
      if (packet.src) pkt.src = packet.src
      if (packet.dst) pkt.dst = packet.dst
      if (packet.nxt) pkt.nxt = packet.nxt
      if (packet.seq !== undefined) pkt.seq = packet.seq
      if (packet.ack !== undefined) pkt.ack = packet.ack
      if (packet.ctx !== undefined) pkt.ctx = packet.ctx
      if (packet.typ !== undefined) pkt.typ = packet.typ
      if (packet.data) pkt.data = packet.data
      this.ws.send(pkt.toJson())
    }
  }
}
