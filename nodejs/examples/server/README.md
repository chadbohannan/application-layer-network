# ALN Server Example

A demonstration ALN (Application Layer Network) server that accepts both TCP and WebSocket connections for mesh networking.

## Overview

This server hosts an ALN router node that:
- Listens for **TCP connections** on port 8000 (binary KISS framing)
- Listens for **WebSocket connections** on port 8080 (JSON framing)
- Provides a **"ping" service** that responds with "pong"
- Includes a basic HTTP API on port 8080

## Quick Start

### 1. Install Dependencies

From the `examples/` directory:
```bash
npm install
```

### 2. Run the Server

```bash
npm run server
```

Or directly:
```bash
node server/server.js
```

### 3. Configure Ports (Optional)

Use environment variables to change default ports:
```bash
PORT=3000 TCP_PORT=9000 node server/server.js
```

## Usage

### Connect via TCP (Binary Protocol)

Use the simple-client example to connect via TCP:
```bash
cd examples/
node simple-client/client.js
```

This demonstrates:
- Automatic service discovery
- Sending a ping request
- Receiving a pong response

### Connect via WebSocket (JSON Protocol)

WebSocket connections use JSON encoding for human-readable debugging:

```javascript
const WebSocket = require('ws')
const ws = new WebSocket('ws://localhost:8080')

ws.on('open', () => {
  // Send ping request using JSON format
  ws.send(JSON.stringify({
    srv: 'ping',
    ctx: 12345,
    data: Buffer.from('hello').toString('base64')
  }))
})

ws.on('message', (data) => {
  const packet = JSON.parse(data)
  console.log('Received:', packet)
})
```

### HTTP API

The server also provides a basic HTTP endpoint:

```bash
curl http://localhost:8080/
# Returns: {"user":"none"}
```

## Architecture

```
┌─────────────────────────────────────────┐
│          ALN Server (nodejs-host)       │
├─────────────────────────────────────────┤
│                                         │
│  ┌──────────┐         ┌─────────────┐  │
│  │   HTTP   │         │  ALN Router │  │
│  │   API    │         │  (ping svc) │  │
│  │ :8080    │         └──────┬──────┘  │
│  └──────────┘                │         │
│                               │         │
│  ┌──────────┐         ┌──────┴──────┐  │
│  │WebSocket │◄────────┤   Channels  │  │
│  │ :8080    │         │  (TCP/WS)   │  │
│  └──────────┘         └──────┬──────┘  │
│                               │         │
│  ┌──────────┐                │         │
│  │   TCP    │◄───────────────┘         │
│  │  :8000   │                          │
│  └──────────┘                          │
└─────────────────────────────────────────┘
```

## Services Provided

### Ping Service

The server registers a "ping" service that:
1. Receives packets with `service: "ping"`
2. Logs the incoming request
3. Responds to the source address with `data: "pong"`
4. Uses the same `contextID` for response correlation

**Example Request:**
```json
{
  "srv": "ping",
  "ctx": 12345,
  "data": "aGVsbG8="  // base64("hello")
}
```

**Example Response:**
```json
{
  "dst": "<client-address>",
  "ctx": 12345,
  "data": "cG9uZw=="  // base64("pong")
}
```

## Testing

Run the included test:
```bash
npm test
```

This validates:
- HTTP API endpoints
- Server initialization
- Response formatting

## Logging

The server uses Winston for structured logging:
- **Info**: Startup messages, port bindings, connections
- **Console**: Ping service activity

Logs appear in the console with timestamps and levels.

## Protocol Details

### TCP Connections (Port 8000)
- Uses **binary KISS framing** per ALN spec Section 4.1
- Frame delimiter: `0xC0`
- Byte stuffing for special characters
- Suitable for raw socket connections

### WebSocket Connections (Port 8080)
- Uses **JSON encoding** per ALN spec Section 4.2
- Human-readable packet format
- Base64-encoded binary data
- Built-in message framing from WebSocket

## Extending the Server

### Add a New Service

```javascript
// In server.js
alnRouter.registerService('echo', (packet) => {
  const response = new Packet()
  response.dst = packet.src
  response.ctx = packet.ctx
  response.data = packet.data // Echo back the data
  alnRouter.send(response)
})
```

### Add HTTP Endpoints

Edit `routes/index.js`:
```javascript
router.get('/status', (req, res) => {
  res.json({
    address: alnRouter.address,
    channels: alnRouter.channels.length,
    services: Array.from(alnRouter.serviceMap.keys())
  })
})
```

## See Also

- **[simple-client](../simple-client/)** - TCP client example with event-driven service discovery
- **[maln-client](../maln-client/)** - Cloud ALN network client example
- **[ALN Protocol Spec](../../../ALN_PROTOCOL.md)** - Complete protocol documentation
- **[Core Library](../../lib/aln/)** - ALN implementation details

## License

MIT
