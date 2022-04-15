const { Packet } = require("./packet");

class WebSocketChannel {
  constructor (ws) {
    this.ws = ws
    this.onPacket = () => { console.log("WebSocketChannel::onPacket; default")}
    ws.on('message', function message(data) {
      console.log('ws received: %s', data);
      this.onPacket(new Packet(data))
    });
  }

  send(packet) {
    this.ws.send(packet.toBinary())
  }
}

module.exports.WebSocketChannel = WebSocketChannel
