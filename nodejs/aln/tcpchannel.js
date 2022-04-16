const { Parser, toAx25Frame } = require('./parser')
// const { Packet } = require('../aln/packet')

class TcpChannel {
  constructor (socket) {
    const _this = this
    this.onPacket = () => { console.log('TcpChannel::onPacket; default') }
    this.onClose = () => { console.log('TcpChannel::onClose; default') }
    const parser = new Parser((packet) => _this.onPacket(packet, this))
    socket.on('data', function(chunk) {
      console.log(`TcpChannel: on data: ${chunk.length} bytes.`);
      parser.readAx25FrameBytes(chunk.toString('binary'), chunk.length)
    });
    socket.on('end', function() {
        console.log('TcpChannel socket closing');
        _this.onClose()
    });
    socket.on('error', function(err) {
        console.log(`TcpChannel error: ${err}`);
    });
    this.parser = parser
    this.socket = socket
  }

  send(packet) {
    const frame = toAx25Frame(packet.toBinary())
    this.socket.write(frame, 'binary')
  }

  close() {
    console.log('ZTcpChannel::close()')
    this.socket.close()
  }

  // onClose() {
  //   console.log('TcpChannel::onClose; default')
  // }
}

module.exports.TcpChannel = TcpChannel
