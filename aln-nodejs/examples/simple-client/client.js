const net = require('net')
const Router = require('../../lib/aln/router')
const { Packet } = require('../../lib/aln/packet')
const { TcpChannel } = require('../../lib/aln/tcpchannel')

const TCP_PORT = process.env.TCP_PORT || 8001

const alnRouter = new Router('nodejs-client')

// setup a response handler
const ctx = alnRouter.registerContextHandler((p) => {
  console.log('ping response:', p.data)
  process.exit(0) // exit after receiving response
})

alnRouter.setOnServiceCapacityChanged((service, capacity, address) => {
  console.log(`Service discovered: ${service} at ${address} (capacity: ${capacity})`)

  if (service === 'ping' && capacity > 0) {
    console.log('ping service discovered; sending ping')
    const pingPacket = new Packet()
    pingPacket.srv = 'ping'
    pingPacket.ctx = ctx
    alnRouter.send(pingPacket)
  }
})

const socket = new net.Socket()
socket.connect({ port: TCP_PORT, host: '127.0.0.1' }, function () {
  console.log('channel connected')
})

const tcpChannel = new TcpChannel(socket)
alnRouter.addChannel(tcpChannel)

console.log('init complete - waiting for service discovery...')
