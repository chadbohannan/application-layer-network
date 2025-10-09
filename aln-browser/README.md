# ALN Browser Client

Browser implementation of the Application Layer Network protocol using WebSocket and native browser APIs.

**Zero dependencies** • ES6 modules • JSON framing • WebSocket-only

## Installation

```bash
npm install aln-browser
```

## Quick Start

```javascript
import { Router } from 'aln-browser';
import { WebSocketChannel } from 'aln-browser/wschannel';

// Create router with unique address
const address = 'browser-client-' + Math.random().toString(36).slice(2, 8);
const router = new Router(address);

// Connect to ALN WebSocket server
const ws = new WebSocket('ws://localhost:8080');

ws.onopen = () => {
    const channel = new WebSocketChannel(ws);
    router.addChannel(channel);
    console.log('Connected to ALN network');
};

// Register a log capture service
router.registerService('log', (packet) => {
    console.log('logging: ' + packet.data)
});

// Register a ping capture service
router.registerService('ping', (packet) => {
    console.log('pong: ' + packet.data)
});

// Send to all instances of the 'log' service on the network
router.send({
    srv: 'log',
    data: 'Hello World!'
});

// Send to a specific instance of the logging service
router.send({
    srv: 'log',
    dst: address,
    data: 'Hello World!'
});

// To capture a response from a service, set up a context handler
// use ctxID in your service request, the service must then copy
// the ctx from the request packet to the return packet so it is
// routed all the way to the correct handler.
const ctxID = router.registerContextHandler((response) => {
    console.log(`Response from ${serviceID}@${address}: ${data}`);
    router.releaseContext(ctxID); // free some memory
});

// Send to a specific instance of the logging service with a reference
// to a response handler
router.send({
    srv: 'ping',
    dst: address,
    ctx: ctxID,
    data: 'tag, yer it!'
});

```

## Running the Example

The package includes an interactive demo. To run it:

```bash
# Start HTTP server (required for ES6 modules)
python3 -m http.server 8000

# In another terminal, start an ALN WebSocket server
# Node.js option:
cd node_modules/aln-browser/../../../nodejs/examples/server
npm install && npm start

# Go option:
cd examples/go-server
go run main.go
```

Open http://localhost:8000/examples/ping-client.html

**Note:** Files must be served over HTTP, not opened directly (`file://` protocol doesn't support ES6 modules).

## API

### Router

```javascript
new Router(address: string)
```

Create a router with a unique address.

**Methods:**
- `addChannel(channel)` - Attach a WebSocket channel
- `send(packet)` - Send a packet (auto-routes to destination)
- `registerService(serviceID, handler)` - Handle incoming requests
- `registerContextHandler(handler)` - Returns contextID for responses
- `releaseContext(ctxID)` - Free context handler (prevents memory leaks)

### WebSocketChannel

```javascript
new WebSocketChannel(websocket: WebSocket)
```

Wraps a browser WebSocket for ALN routing.

### Packet Format

Packets are plain JavaScript objects:

```javascript
{
    srv: 'service-name',    // Service to invoke
    src: 'source-addr',     // Source address (auto-filled)
    dst: 'dest-addr',       // Destination address (optional if srv is set)
    ctx: 12345,             // Context ID for request-response
    data: 'payload'         // Arbitrary data
}
```

## Architecture

- **WebSocket Only**: No TCP/Serial support (browser limitation)
- **JSON Framing**: WebSocket provides message boundaries, no KISS encoding needed
- **Native APIs**: Uses TextEncoder, DataView, atob/btoa for binary handling
- **Service Discovery**: Automatic advertisement and routing to services
- **Distance Vector Routing**: Multi-hop packet routing with cost metrics

## Development

```bash
# Run tests (Node.js)
npm test

# Run tests (browser)
python3 -m http.server 8000
# Open http://localhost:8000/examples/test.html

# Serve examples
npm run serve
```

All 41 tests passing ✅

## Browser Compatibility

Requires:
- WebSocket API
- ES6 modules
- TextEncoder/TextDecoder

Works on Chrome 90+, Firefox 88+, Safari 14+, Edge 90+

## Security

**Development:** WebSocket servers typically allow all origins for testing.

**Production:**
- Validate WebSocket origin headers
- Use WSS (secure WebSocket) when serving over HTTPS
- Restrict connections to trusted domains

## License

MIT
