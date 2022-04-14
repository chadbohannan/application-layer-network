const app = require('./app')
const http = require('http')
const net = require('net')
const logger = require('./logging/logger')
const Router = require('./aln/router')
const { TcpChannel } = require('./aln/tcpchannel')

const PORT = process.env.PORT || 8080
const TCP_PORT = process.env.TCP_PORT || 8181

const alnRouter = new Router('nodejs-host')
alnRouter.registerService(1, (packet) => { console.log('packet recieved', packet.toJson()) })

const tcpServer = net.createServer()
tcpServer.listen(TCP_PORT, () => {
  logger.info(`socket host on port ${TCP_PORT}`)
})
tcpServer.on('connection', function(socket) {
  alnRouter.addChannel(new TcpChannel(socket))
  console.log('A new connection has been established.');
});

const webServer = http.createServer(app)
webServer.listen(PORT, () => {
  logger.info(`web server on port ${PORT}`)
})

logger.info('init complete')
