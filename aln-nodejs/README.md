# aln-nodejs

Application Layer Network protocol implementation for Node.js - A mesh networking library for distributed service discovery and communication.

[![npm version](https://badge.fury.io/js/aln-nodejs.svg)](https://www.npmjs.com/package/aln-nodejs)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Features

- **Service Discovery**: Automatic advertisement and discovery of network services
- **Distance-Vector Routing**: Multi-hop routing with automatic route updates
- **Load Balancing**: Service selection based on capacity metrics
- **Mesh Networking**: Peer-to-peer communication across arbitrary network topologies
- **Transport Agnostic**: Works over TCP (binary KISS framing) and WebSocket (JSON encoding)
- **Request/Response Pattern**: Context-based message correlation
- **Service Multicast**: Send to all instances of a service

## Installation

```bash
npm install aln-nodejs
```

**Dependencies:**
- `bytebuffer` - Binary packet serialization
- `ws` - WebSocket transport

## Quick Start

```javascript
const { WebSocketServer } = require('ws')
const app = require('./app') // see example for sample app
const Router = require('aln-nodejs')
const { Packet } = require('aln-nodejs/lib/aln/packet')
const { TcpChannel } = require('aln-nodejs/lib/aln/tcpchannel')

const { WebSocketChannel } = require('aln-nodejs/lib/aln/wschannel')


// Create a router with a unique address
const address = 'nodejs-host-' + Math.random().toString(36).slice(2, 8);
const router = new Router(address);

// Register a service
router.registerService('ping', (packet) => {
  const response = new Packet()
  response.dst = packet.src // send the packet back where it came from
  response.ctx = packet.ctx // specify the destination's context handler
  response.data = 'pong: ' + packet.data // compose a response including recieved payload
  router.send(response)
})

// boiler-plate http server creation
const webServer = http.createServer(app)
webServer.listen(WS_PORT, () => {
  log(`web server on port ${WS_PORT}`)
})

const wss = new WebSocketServer({ noServer: true })
wss.on('connection', function connection (ws) {
  alnRouter.addChannel(new WebSocketChannel(ws))
  log('WebSocket connection established')
})

webServer.on('upgrade', function upgrade (request, socket, head) {
  wss.handleUpgrade(request, socket, head, function done (ws) {
    wss.emit('connection', ws, request)
  })
})

```

## API Reference

### Router

**Constructor:**
```javascript
const router = new Router(address)
```
- `address` (string): Unique identifier for this router node

**Methods:**

#### `registerService(serviceID, handler)`
Register a service handler for incoming requests.
- `serviceID` (string): Service name
- `handler` (function): Callback function `(packet) => {}`

#### `unregisterService(serviceID)`
Remove a service handler.

#### `send(packet)`
Send a packet through the network.
- Returns: `null` on success, error string on failure

#### `addChannel(channel)`
Attach a transport channel to the router.

#### `registerContextHandler(handler)`
Register a response handler for request/response pattern.
- Returns: `contextID` (number) to use in outgoing request

#### `releaseContext(contextID)`
Free memory associated with a context handler.

#### `setOnServiceCapacityChanged(callback)`
Register callback for service capacity changes.
- `callback` signature: `(serviceID, capacity, address) => {}`

### Packet

**Constructor:**
```javascript
const packet = new Packet()
```

**Properties:**
- `src` (string): Source address
- `dst` (string): Destination address
- `srv` (string): Service name
- `ctx` (number): Context ID for request/response
- `data` (string): Packet payload
- `nxt` (string): Next hop address (managed by router)
- `net` (number): Network state type
- `seq`, `ack`, `typ`: Optional fields

**Methods:**
- `toBinary()`: Serialize to binary format
- `toJson()`: Serialize to JSON format
- `copy()`: Create a deep copy

### TcpChannel

```javascript
const net = require('net')
const { TcpChannel } = require('aln-nodejs/lib/aln/tcpchannel')

const socket = net.connect(port, host)
const channel = new TcpChannel(socket)
router.addChannel(channel)
```

### WebSocketChannel

```javascript
const WebSocket = require('ws')
const { WebSocketChannel } = require('aln-nodejs/lib/aln/wschannel')

const ws = new WebSocket('ws://localhost:8080')
const channel = new WebSocketChannel(ws)
router.addChannel(channel)
```

## Examples

This repository includes example applications in the `examples/` directory:

### Running the Examples

```bash
# Clone the repository
git clone https://github.com/chadbohannan/application-layer-network.git
cd application-layer-network/aln-nodejs/examples
npm install

# Start the server (TCP port 8001, HTTP/WebSocket port 8080)
npm run server

# In another terminal, run a client
npm run simple-client

# Or run the multiplexed ALN client
npm run maln-client
```

### Example: Service Discovery

```javascript
const Router = require('aln-nodejs')
const { Packet } = require('aln-nodejs/lib/aln/packet')

const router = new Router('service-node')

// Register a service
router.registerService('echo', (packet) => {
  console.log('Echo request:', packet.data)

  const response = new Packet()
  response.dst = packet.src
  response.ctx = packet.ctx
  response.data = packet.data // Echo back
  router.send(response)
})

// Service discovery happens automatically when channels are added
```

### Example: Request/Response

```javascript
const Router = require('aln-nodejs')
const { Packet } = require('aln-nodejs/lib/aln/packet')

const router = new Router('client-node')

// Register response handler
const ctxID = router.registerContextHandler((responsePacket) => {
  console.log('Received response:', responsePacket.data)
  router.releaseContext(ctxID) // Clean up
})

// Send request
const request = new Packet()
request.srv = 'echo' // Target service
request.ctx = ctxID
request.data = 'Hello!'
router.send(request)
```

### Example: Service Multicast

```javascript
const packet = new Packet()
packet.srv = 'echo' // Target all 'echo' services
// Don't set packet.dst - multicast requires empty destination
packet.data = 'Broadcast message'
router.send(packet)
// All instances of 'echo' service will receive this packet
```

## Core Concepts

### Router
The central component that manages:
- Service registration and discovery
- Distance-vector routing
- Multi-hop packet forwarding
- Load-based service selection

### Channels
Transport-agnostic connections:
- **TcpChannel**: Stream-based binary KISS framing
- **WebSocketChannel**: JSON-encoded packets (human-readable)

### Services
Named endpoints that handle application requests:
- Automatic service discovery
- Load balancing across multiple instances
- Service multicast support

## Testing

```bash
npm test          # Run all tests with coverage
npm run lint      # Check code style
```

## Protocol

See [ALN_PROTOCOL.md](https://github.com/chadbohannan/application-layer-network/blob/master/ALN_PROTOCOL.md) for complete protocol specification.

## Related Projects

- **aln-browser**: Browser WebSocket client implementation
- **aln-python**: Python implementation
- **aln-go**: Go implementation

## License

MIT - See LICENSE file for details

## Author

Chad Bohannan

## Contributing

Issues and pull requests welcome at https://github.com/chadbohannan/application-layer-network
