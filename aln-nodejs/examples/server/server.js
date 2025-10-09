const app = require('./app')
const http = require('http')
const net = require('net')
const logger = require('./logging/logger')
const Router = require('../../lib/aln/router')
const { TcpChannel } = require('../../lib/aln/tcpchannel')
const { WebSocketChannel } = require('../../lib/aln/wschannel')
const { WebSocketServer } = require('ws')
const { Packet } = require('../../lib/aln/packet')

const WS_PORT = process.env.PORT || 8080
const TCP_PORT = process.env.TCP_PORT || 8001

const alnRouter = new Router('nodejs-host-' + process.pid.toString(16).slice(0, 6).padStart(6, '0'))
alnRouter.registerService('ping', (packet) => {
  logger.debug('ping handler for:', packet.toJson())
  const pongPacket = new Packet()
  pongPacket.dst = packet.src
  pongPacket.ctx = packet.ctx
  pongPacket.data = 'pong:' + (packet.data || '')
  alnRouter.send(pongPacket)
  logger.debug('"pong" returned')
})

const tcpServer = net.createServer()
tcpServer.listen(TCP_PORT, () => {
  logger.info(`socket host on port ${TCP_PORT}`)
})
tcpServer.on('connection', function (socket) {
  alnRouter.addChannel(new TcpChannel(socket))
  logger.info('TCP connection established')
})

const webServer = http.createServer(app)
webServer.listen(WS_PORT, () => {
  logger.info(`web server on port ${WS_PORT}`)
})

const wss = new WebSocketServer({ noServer: true })
wss.on('connection', function connection (ws) {
  alnRouter.addChannel(new WebSocketChannel(ws))
  logger.info('WebSocket connection established')
})

webServer.on('upgrade', function upgrade (request, socket, head) {
  wss.handleUpgrade(request, socket, head, function done (ws) {
    wss.emit('connection', ws, request)
  })
})

logger.info('init complete')
