import ByteBuffer from 'bytebuffer'
import { Packet } from './packet'

export class WebSocketChannel {
  constructor (ws) {
    this.ws = ws
    this.onPacket = () => { console.log("WebSocketChannel::onPacket; default")}
    this.onClose = () => { console.log("WebSocketChannel::onClose; default")}

    ws.onmessage = (event) => {
      console.log('ws received: %s', event.data);
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
      if (obj.data) packet.data = ByteBuffer.fromBase64(obj.data).toBinary()
      this.onPacket(packet)
    };
    ws.onerror = (event) => {
        console.log(event);
    };
    
    ws.onclose = (event) => {
        console.log(event);
    };
    
  }

  send(packet) {
    console.log('sending:', packet.toJson())
    this.ws.send(packet.toJson())
  }
}
