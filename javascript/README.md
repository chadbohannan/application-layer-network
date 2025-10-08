# ALN Browser Client (JavaScript)

Browser-only implementation of the Application Layer Network protocol using WebSocket.

## Features

- WebSocket-only transport (no TCP/Serial)
- JSON packet framing (no KISS encoding needed)
- Browser WebSocket API
- ES6 modules
- **Zero dependencies** - uses native browser APIs only

## Usage

### 1. Install Dev Dependencies (Optional)

```bash
npm install
```

This installs dev dependencies for testing (Chai). **No runtime dependencies needed** - the library uses only native browser APIs!

### 2. Run the Example

**IMPORTANT:** You MUST serve the files over HTTP (not `file://`). ES6 modules don't work when opening HTML files directly.

Start a local HTTP server:

```bash
npm run serve
# or
python3 -m http.server 8000
```

**Don't open the HTML file directly!** Always access via `http://localhost:8000/`

### 3. Start WebSocket Server

You need an ALN WebSocket server. Choose one:

#### Option A: Node.js Server (Recommended - Same Repo)

```bash
cd ../nodejs/examples/server
npm install
npm run server
```

This starts:
- WebSocket server on port 8080 (for browser clients)
- TCP server on port 8000 (for other clients)

#### Option B: Go Server

```bash
cd ../go/examples/websocket_host
go mod tidy
go run main.go
```

This starts:
- WebSocket server on port 8082 (for browser clients)
- TCP server on port 8081 (for other clients)

#### Option C: Python Server

If a Python WebSocket example exists, it can also be used.

### 4. Open Browser

Navigate to:
- Ping Example: http://localhost:8000/examples/ping-client.html
- Unit Tests: http://localhost:8000/examples/test.html

**Note:** Update the WebSocket URL in ping-client.html to match your server:
- Node.js server: `ws://localhost:8080`
- Go server: `ws://localhost:8082`

## Architecture

### Files

- `aln/packet.js` - Packet serialization/deserialization
- `aln/router.js` - Core routing logic
- `aln/wschannel.js` - WebSocket channel implementation
- `aln/utils.js` - Utility functions for network state packets
- `aln/binary-utils.js` - Native browser API wrappers for binary data

### Key Differences from Node.js Implementation

1. **WebSocket Only**: No TCP or serial support
2. **Browser WebSocket API**: Uses native browser WebSocket, not `ws` library
3. **JSON Framing**: WebSocket messages are JSON-encoded packets (no KISS framing)
4. **JSON-Only Serialization**: Binary serialization removed (not needed for WebSocket)
5. **ES6 Modules**: Uses native ES6 imports (not CommonJS require)

## Status

✅ **All critical bugs fixed** (2025-10-07)
✅ **41/41 tests passing** (updated 2025-10-08)
✅ **Ready for production use**

See [issues.md](issues.md) for complete compliance status and known limitations.

## Testing

Open `examples/test.html` in a browser to run unit tests.

The tests cover:
- Packet serialization
- Router functionality
- Service discovery
- Route learning
- Utility functions

## Development

### Code Style

- ES6+ JavaScript
- Modules (import/export)
- camelCase naming
- No semicolons (except where required)

### Building for Production

For production use, you may want to bundle the code:

```bash
# Using Rollup
rollup aln/router.js --file dist/aln.bundle.js --format iife

# Using Webpack
webpack --mode production
```

## Browser Compatibility

Requires:
- WebSocket API support
- ES6 modules support
- Fetch API (for future features)

Tested on:
- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

## Security Considerations

### CORS
The WebSocket server must allow connections from your domain. The Go example uses `CheckOrigin: func(r *http.Request) bool { return true }` for development.

### HTTPS/WSS
If your webpage is served over HTTPS, you must use WSS (secure WebSocket):
```javascript
const ws = new WebSocket('wss://example.com:8082');
```

### Origin Validation
Production servers should validate the WebSocket origin header.

## Examples

### Basic Connection

```javascript
import { Router } from './aln/router.js';
import { WebSocketChannel } from './aln/wschannel.js';

const router = new Router('my-browser-client');
const ws = new WebSocket('ws://localhost:8082');

ws.onopen = () => {
    const channel = new WebSocketChannel(ws);
    router.addChannel(channel);
};
```

### Sending a Packet

```javascript
const packet = {
    srv: 'my-service',
    data: 'Hello, ALN!'
};

router.send(packet);
```

### Registering a Service

```javascript
router.registerService('echo', (packet) => {
    console.log('Received:', packet.data);

    const response = {
        dst: packet.src,
        ctx: packet.ctx,
        data: packet.data
    };

    router.send(response);
});
```

### Request-Response Pattern

The context handler pattern enables request-response communication:

```javascript
// Register handler before sending request
const ctxID = router.registerContextHandler((response) => {
    console.log('Got response:', response.data);

    // User's responsibility to release when done
    // For single-response services:
    router.releaseContext(ctxID);

    // For multi-response services (streaming, pagination):
    // if (isComplete) { router.releaseContext(ctxID); }
});

// Send request with context ID
router.send({
    srv: 'some-service',
    ctx: ctxID,
    data: 'request data'
});

// IMPORTANT: Always release contexts to prevent memory leaks
// The handler may be called multiple times before release
```

**Key Points:**
- Context handlers can receive **multiple responses** for a single request
- **You must release** contexts when done: `router.releaseContext(ctxID)`
- Failure to release contexts causes **memory leaks**
- Context IDs are unique per router instance
- Valid context IDs: 1-65535 (0 is reserved)

## License

MIT
