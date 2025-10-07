const net = require('net')
const Router = require('../../lib/aln/router')
const { Packet } = require('../../lib/aln/packet')
const { TcpChannel } = require('../../lib/aln/tcpchannel')

    // decomposed exmample URL; layer7node requires user registration
    // tcp+maln://layer7node.net:8000/6b404c2d-50d1-4007-94af-1b157c64e4e3

const TCP_HOST = 'layer7node.net'
const TCP_PORT = 8000
const ALN_NODE = '6b404c2d-50d1-4007-94af-1b157c64e4e3'

const localAddress = 'nodejs-client-' + Math.random().toString(36).slice(2, 7);
const alnRouter = new Router(localAddress);
console.log('localAddress: ' + localAddress);

alnRouter.registerService('ping', (packet) => {
  console.log('ping handler for:', packet.toJson())
  const pongPacket = new Packet()
  pongPacket.dst = packet.src
  pongPacket.ctx = packet.ctx
  pongPacket.data = 'pong'
  alnRouter.send(pongPacket)
  console.log('"pong" returned')
})




const socket = new net.Socket()
socket.connect({ port: TCP_PORT, host: TCP_HOST }, function () {
  console.log('connection success')

  setTimeout(() => {
    console.log('sending ping')
    const logPacket = new Packet()
    logPacket.srv = 'ping'
    logPacket.data = 'hello'
    alnRouter.send(logPacket)
  }, 1000) // wait for protocol to settle
})

const tcpChannel = new TcpChannel(socket)

// the first packet must address the ALN cloud node to finish connecting
// this socket to a specific ALN network hosted on the web before our router
// can synchronize state with the cloud node
console.log('sending multiplexed ALN selection packet')
const pingPacket = new Packet()
pingPacket.dst = ALN_NODE
tcpChannel.send(pingPacket)

// let our router manage this connection from here on
alnRouter.addChannel(tcpChannel)

console.log('init complete, awaiting connection')
console.log('cntl+C at any time to exit')
