const net = require('net')
const logger = require('./logging/logger')
const Router = require('./aln/router')
const { Packet } = require('./aln/packet')
const { TcpChannel } = require('./aln/tcpchannel')

const TCP_PORT = process.env.TCP_PORT || 8181

const alnRouter = new Router('nodejs-client')

const socket = new net.Socket()
socket.connect({ port: TCP_PORT, host: 'localhost' }, function () {
  setTimeout(() => {
    console.log('TCP connection established with the server.')
    const packet = new Packet()
    packet.srv = 1
    alnRouter.send(packet)
  }, 100) // wait for protocol to settle
})

const tcpChannel = new TcpChannel(socket)
alnRouter.addChannel(tcpChannel)

logger.info('init complete')

logger.info('sending ping')
const ctx = alnRouter.registerContextHandler((p) => {
  console.log('ping response:', p.data)
})
const pingPacket = new Packet()
pingPacket.srv = 'ping'
pingPacket.ctx = ctx
alnRouter.send(pingPacket)
