const net = require('net')
const logger = require('./logging/logger')
const Router = require('./aln/router')
const { Packet } = require('./aln/packet')
const { TcpChannel } = require('./aln/tcpchannel')

const TCP_PORT = process.env.TCP_PORT || 8000

const alnRouter = new Router('nodejs-client')

// setup a response handler
const ctx = alnRouter.registerContextHandler((p) => {
  console.log('ping response:', p.data)
})

const socket = new net.Socket()
socket.connect({ port: TCP_PORT, host: '127.0.0.1' }, function () {
  logger.info('channel connected')
  setTimeout(() => {
    logger.info('sending ping')
    const pingPacket = new Packet()
    pingPacket.srv = 'ping'
    pingPacket.ctx = ctx
    alnRouter.send(pingPacket)
  }, 100) // wait for protocol to settle
})

const tcpChannel = new TcpChannel(socket)
alnRouter.addChannel(tcpChannel)

logger.info('init complete')
