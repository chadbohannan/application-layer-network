# ALN - Application Layer Network (Node.js)

A mesh networking protocol for distributed service discovery and communication.

## Project Structure

```
nodejs/
├── lib/aln/              # Core ALN library
│   ├── packet.js         # Packet structure and serialization
│   ├── parser.js         # KISS frame parser
│   ├── router.js         # Core routing logic
│   ├── utils.js          # Network state packet helpers
│   ├── tcpchannel.js     # TCP transport channel
│   └── wschannel.js      # WebSocket transport channel
│
├── examples/             # Demo applications
│   ├── simple-client/    # Basic TCP client example
│   ├── maln-client/      # Multiplexed ALN client
│   └── server/           # Server with TCP & WebSocket support
│
├── tests/                # Unit tests
└── package.json          # Minimal library dependencies
```

## Installation

### Core Library
```bash
npm install
```

**Dependencies:**
- `bytebuffer` - Binary packet serialization
- `ws` - WebSocket transport

### Examples (Optional)
```bash
cd examples
npm install
```

## Usage

### Import the Library

```javascript
const Router = require('./lib/aln/router')
const { Packet } = require('./lib/aln/packet')
const { TcpChannel } = require('./lib/aln/tcpchannel')
const { WebSocketChannel } = require('./lib/aln/wschannel')
```

### Basic Routing Example

```javascript
const Router = require('./lib/aln/router')
const { Packet } = require('./lib/aln/packet')

// Create a router with a unique address
const router = new Router('my-node')

// Register a service
router.registerService('ping', (packet) => {
  console.log('Received ping from:', packet.src)

  const response = new Packet()
  response.dst = packet.src
  response.ctx = packet.ctx
  response.data = 'pong'
  router.send(response)
})

// Add a transport channel (TCP, WebSocket, etc.)
router.addChannel(channel)
```

### Run Examples

From the `examples/` directory:

```bash
# Start the server (TCP port 8000, HTTP/WebSocket port 8080)
npm run server

# In another terminal, run a client
npm run simple-client

# Or run the multiplexed ALN client
npm run maln-client
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

### Packets
Messages exchanged over the network:
- Network state (routes, services, queries)
- Application data
- Request/response via context IDs

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

See [ALN_PROTOCOL.md](../ALN_PROTOCOL.md) for complete protocol specification.

## License

MIT
