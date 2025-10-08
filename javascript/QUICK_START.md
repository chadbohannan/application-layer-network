# Quick Start - JavaScript ALN Browser Client

## ⚠️ Common Issue: CORS Errors

If you see errors like:
```
Access to script at 'file:///.../router.js' from origin 'null' has been blocked by CORS policy
```

**Problem:** You opened the HTML file directly (`file://` protocol)

**Solution:** Use a web server (HTTP protocol)

---

## Step-by-Step Setup

### 1. Start Web Server for JavaScript Files

This step is nessessary because the javascript needs to runn from an HTTP address,
i.e. http://localhost:8000/examples/ping-client.html. Running from the file:// protocol
causes a CORS violation as part of keeping your computer safe.

From the `javascript/` directory:

```bash
python3 -m http.server 8000
```

**Keep this running!**

### 2. Start WebSocket Server

**Option A: Node.js**

New terminal:
```bash
cd ../nodejs/examples/server
npm install
npm run server
```

Server runs on `ws://localhost:8080`

**Option B: Go**

New terminal:
```bash
cd ../go/examples/websocket_host
go mod tidy
go run main.go
```

Server runs on `ws://localhost:8082`

### 3. Open Browser

Navigate to: **http://localhost:8000/examples/ping-client.html**

### 4. Test Connection

1. The WebSocket URL should show `ws://localhost:8080` (Node.js) or change to `ws://localhost:8082` (Go)
2. Click **"Connect"**
3. Wait for "Connected" status
4. Click **"Send Ping"**
5. See "Received response: pong..." in the log

---

## Troubleshooting

### CORS Errors
- ❌ Opening HTML file directly
- ✅ Must use `http://localhost:8000/...`

### Connection Refused
- Check server is running
- Verify port matches (8080 for Node.js, 8082 for Go)
- Check firewall settings

### No Response to Ping
- Look at server logs
- Check "Network State" section for discovered services
- Verify routes were learned

### Module Not Found
- Ensure you're in `javascript/` directory when running `python3 -m http.server`
- Check browser console for exact path errors
- Hard refresh browser (Ctrl+Shift+R or Cmd+Shift+R) to clear cache

---

## Running Tests

Unit tests can run via Node.js (recommended):

```bash
cd javascript
npm install  # Install dev dependencies (chai)
npm test
```

Note: The library itself has **zero dependencies** and uses only native browser APIs!

Or in browser:
```
http://localhost:8000/examples/test.html
```

---

## File Structure

```
javascript/
├── aln/
│   ├── packet.js      # Packet serialization
│   ├── router.js      # Core routing logic
│   ├── wschannel.js   # WebSocket channel
│   └── utils.js       # Utility functions
├── examples/
│   ├── ping-client.html  # Interactive demo ← OPEN THIS
│   └── test.html         # Browser tests
└── tests/
    └── *.test.js      # Node.js unit tests
```

---

## Next Steps

- Modify ping-client.html to add your own services
- Check out the test files for API examples
- Read README.md for detailed documentation
- Review issues.md for known limitations
