const app = require('./app')
const http = require('http')
const net = require('net')
const logger = require('./logging/logger')
const Router = require('../../lib/aln/router')
const { TcpChannel } = require('../../lib/aln/tcpchannel')
const { WebSocketChannel } = require('../../lib/aln/wschannel')
const { WebSocketServer } = require('ws')
const { Packet } = require('../../lib/aln/packet')

const PORT = process.env.PORT || 8080
const TCP_PORT = process.env.TCP_PORT || 8000

const alnRouter = new Router('nodejs-host')
alnRouter.registerService('ping', (packet) => {
  console.log('ping handler for:', packet.toJson())
  const pongPacket = new Packet()
  pongPacket.dst = packet.src
  pongPacket.ctx = packet.ctx
  pongPacket.data = 'pong'
  alnRouter.send(pongPacket)
  console.log('"pong" returned')
})

const tcpServer = net.createServer()
tcpServer.listen(TCP_PORT, () => {
  logger.info(`socket host on port ${TCP_PORT}`)
})
tcpServer.on('connection', function (socket) {
  alnRouter.addChannel(new TcpChannel(socket))
  console.log('A new TCP connection has been established.')
})

const webServer = http.createServer(app)
webServer.listen(PORT, () => {
  logger.info(`web server on port ${PORT}`)
})

const wss = new WebSocketServer({ noServer: true })
wss.on('connection', function connection (ws) {
  alnRouter.addChannel(new WebSocketChannel(ws))
})

webServer.on('upgrade', function upgrade (request, socket, head) {
  wss.handleUpgrade(request, socket, head, function done (ws) {
    wss.emit('connection', ws, request)
  })
})

logger.info('init complete')
