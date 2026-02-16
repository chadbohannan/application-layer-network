# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Application Layer Network (ALN) is a mesh networking protocol with implementations in Go, Node.js, browser JavaScript, Qt/C++, Python, Java, and Arduino. It provides service discovery, distance-vector routing, and multi-hop mesh networking over TCP, TLS, WebSocket, Bluetooth, and LoRa transports. The protocol spec is in `ALN_PROTOCOL.md`.

## Build & Test Commands

### Go
```bash
cd go/aln && go test ./...                    # run all Go tests
cd go/aln && go test -run TestName ./...      # run a single test
cd go/examples/local_host && go run main.go   # run example
```
Go workspace file (`go.work`, Go 1.22) links `./go/aln` and example modules. Examples use `replace` directives pointing to the local `../../aln/` module.

### Node.js
```bash
cd aln-nodejs && npm install && npm test      # lint (standard) + tests (mocha) + coverage (nyc)
cd aln-nodejs && npx mocha tests/test_router.js  # single test file
cd aln-nodejs && npx standard --fix           # auto-fix lint
```

### Browser
```bash
cd aln-browser && npm install && npm test     # tests via Node's native test runner
cd aln-browser && npm run build               # esbuild → dist/ (IIFE + ESM bundles)
cd aln-browser && npm run test:watch          # watch mode
```
Zero production dependencies; ES6 modules only.

### Qt/C++
```bash
cd aln-qt && qmake aln-qt.pro && make        # build all subprojects
cd aln-qt/aln-cli-qt && ./aln-cli-qt         # run CLI tests (QTestLib)
```
Requires Qt 6.5+, C++17. Subdirs project: `aln-cli-qt` (CLI tests), `aln-wrench-qt` (GUI). Shared core in `aln-cpp-core/`.

### Python
```bash
cd python && pip install -e . && pytest tests/ -v   # install + test
```
Package name: `alnmeshpy`. Build system: hatchling. Python ≥3.8.

## Architecture

**Core pattern across all implementations:**
- **Router** — central hub managing channels, service handlers, and routing tables
- **Channel** — transport-agnostic interface wrapping a bytestream (TCP, WebSocket, etc.)
- **Packet** — binary-serialized message with control flags (16-bit, Hamming ECC), service name, source/destination addresses, context ID, and data payload
- **Parser** — KISS-style frame decoder (0xC0 delimiter, 0xDB escape); browser uses JSON framing instead
- **Services** — named packet handlers registered on a router, advertised to the network for discovery
- **Context handlers** — callbacks keyed by context ID for request/response correlation

**Channel decorators** (Go): `ViaChannel` (static routing via gateway), `LimitedChannel` (bandwidth throttling) wrap other channels.

**Routing**: Distance-vector with hop-count metric. Routers exchange route advertisements. Service selection is load-balanced using a 0–65535 load metric.

## Key Source Locations

| Component | Go | Node.js | Browser | C++ |
|-----------|-----|---------|---------|-----|
| Router | `go/aln/router.go` | `aln-nodejs/lib/aln/router.js` | `aln-browser/aln/router.js` | `aln-qt/aln-cpp-core/src/router.cpp` |
| Packet | `go/aln/packet.go` | `aln-nodejs/lib/aln/packet.js` | `aln-browser/aln/packet.js` | `aln-qt/aln-cpp-core/src/packet.cpp` |
| Parser | `go/aln/parser.go` | `aln-nodejs/lib/aln/parser.js` | — (JSON only) | `aln-qt/aln-cpp-core/src/parser.cpp` |
| Types | `go/aln/types.go` | `aln-nodejs/lib/aln/utils.js` | `aln-browser/aln/utils.js` | `aln-qt/aln-cpp-core/include/aln/alntypes.h` |

## Cross-Implementation Consistency

All implementations must follow `ALN_PROTOCOL.md`. When modifying packet format, framing, or routing logic, the same change should be reflected across all language implementations to maintain interoperability.
