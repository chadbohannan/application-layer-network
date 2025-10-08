# WebSocket Host Example

This example creates an ALN router that accepts both WebSocket and TCP connections.

## Features

- WebSocket server on port 8082 (for browser clients)
- TCP server on port 8081 (for Go/Node.js/Python clients)
- Implements "ping" service that echoes responses

## Running

```bash
go run main.go
```

## Testing with Browser Client

1. Start this server:
   ```bash
   cd go/examples/websocket_host
   go run main.go
   ```

2. Serve the JavaScript example:
   ```bash
   cd javascript
   python3 -m http.server 8000
   ```

3. Open browser to http://localhost:8000/examples/ping-client.html

4. Click "Connect" and then "Send Ping"

## Testing with TCP Client

```bash
cd go/examples/local_client
go run main.go
```

The WebSocket and TCP clients can communicate through the router!
