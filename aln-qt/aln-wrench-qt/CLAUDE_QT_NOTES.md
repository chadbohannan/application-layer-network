# ALN Wrench Qt - Codebase Summary

**Purpose**: Qt/C++ implementation of Application Layer Network (ALN) protocol with a single-window interface for network visualization and packet management.

**Status**: Rough prototype application; front-end scheduled for rework.

---

## Project Structure

```
aln-wrench-qt/
├── aln/                          # Core ALN protocol implementation
│   ├── alntypes.{h,cpp}         # Type definitions and byte manipulation utilities
│   ├── packet.{h,cpp}           # ALN packet structure and encoding/decoding
│   ├── frame.{h,cpp}            # KISS framing (character stuffing for packets)
│   ├── parser.{h,cpp}           # Frame parser with escape sequence handling
│   ├── channel.{h,cpp}          # Abstract channel interface
│   ├── tcpchannel.{h,cpp}       # TCP channel implementation
│   ├── localchannel.{h,cpp}     # In-memory channel for testing multi-hop routing
│   └── router.{h,cpp}           # Core routing logic and network state management
├── mainwindow.{h,cpp,ui}        # Main application window
├── connectionitemmodel.{h,cpp}  # Model for active channel connections
├── networkinterfacesitemmodel.{h,cpp}  # Model for network interfaces
├── advertiserthread.{h,cpp}     # UDP broadcast thread for service discovery
├── addchanneldialog.{h,cpp}     # Dialog for manually adding connections
├── packetsenddialog.{h,cpp}     # Dialog for sending packets to services
├── main.cpp                     # Application entry point
└── aln-qt.pro                   # Qt project file
```

**Total Files**: ~29 source/header files

---

## Core Protocol Implementation (aln/)

### 1. Packet Structure (`packet.h/cpp`)

**Key Fields**:
- `net` (char): Network state flag (ROUTE=1, SERVICE=2, QUERY=3)
- `srv` (QString): Service name
- `srcAddress`, `destAddress`, `nxtAddress` (QString): Routing addresses
- `seqNum` (INT16U): Sequence number for reliable transport
- `ackBlock` (INT32U): Acknowledgment block
- `ctx` (INT16U): Context ID for request/response correlation
- `type` (char): Data type indicator
- `data` (QByteArray): Payload (max 1024 bytes)
- `crc` (INT32U): CRC checksum

**Control Flags** (11 bits, Hamming encoded to 15 bits):
- CF_NETSTATE, CF_SERVICE, CF_SRCADDR, CF_DESTADDR, CF_NEXTADDR
- CF_SEQNUM, CF_ACKBLOCK, CF_CONTEXTID, CF_DATATYPE, CF_DATA, CF_CRC

**Encoding**: Hamming (15,11) error correction for control field

**Constructors**:
- `Packet(dest, content)` - Simple packet to destination
- `Packet(dest, contextID, content)` - Request/response packet
- `Packet(dest, service, content)` - Service-addressed packet
- `Packet(QByteArray)` - Parse from wire format

### 2. Router (`router.h/cpp`)

**Purpose**: Central routing logic, service registry, and network state management

**Key Responsibilities**:
- **Packet Routing**: Routes packets based on destination or service name
- **Service Discovery**: Maintains map of services to node addresses
- **Route Management**: Builds/maintains routing table via distance-vector protocol
- **Channel Management**: Handles multiple channels (TCP, local, future: Bluetooth, UDP)

**Data Structures**:
- `remoteNodeMap`: QMap<address, RemoteNodeInfo*> - Routing table
- `serviceCapacityMap`: QMap<service, QMap<address, NodeCapacity*>> - Service availability
- `contextHandlerMap`: QMap<contextID, PacketHandler*> - Request/response tracking
- `serviceHandlerMap`: QMap<serviceName, PacketHandler*> - Local service registry

**Network State Protocol**:
- **NET_ROUTE**: Share routing table updates (address, cost)
- **NET_SERVICE**: Share service availability (address, service, capacity)
- **NET_QUERY**: Request full routing/service table dump

**Routing Algorithm**: Distance-vector with hop count cost metric

**Key Methods**:
- `send(Packet*)`: Route packet locally or to next hop
- `addChannel(Channel*)`: Add new channel and query network state
- `registerService(name, handler)`: Register local service
- `selectServiceAddress(service)`: Load-balance service selection

### 3. Channel Abstraction (`channel.h`)

**Interface**:
- `send(Packet*)`: Transmit packet on channel
- `listen()`: Start receiving packets
- `disconnect()`: Close channel

**Signals**:
- `packetReceived(Channel*, Packet*)`
- `closing(Channel*)`

**Implementations**:
- **TcpChannel**: TCP sockets with frame parser
- **LocalChannel**: In-memory bidirectional channel (testing only)
- **Future**: BluetoothChannel, UdpChannel

### 4. Frame Protocol (`frame.h/cpp`, `parser.h/cpp`)

**Framing**: KISS (Keep It Simple Stupid) framing
- Frame delimiter: `0xC0` (End)
- Escape character: `0xDB` (Esc)
- Escaped End: `0xDC` (EndT)
- Escaped Esc: `0xDD` (EscT)

**Parser**: State machine (STATE_BUFFERING, STATE_ESCAPED) accumulates bytes until frame complete

### 5. Type System (`alntypes.h/cpp`)

**Portable Integer Types**:
- INT08U, INT16U, INT32U (unsigned 8/16/32-bit)

**Utilities**:
- `readINT16U/32U()`: Big-endian byte array to int
- `writeINT16U/32U()`: Int to big-endian byte array
- `writeToBuffer()`: Overloads for QBuffer serialization

---

## UI Architecture (mainwindow.*)

### Layout Overview

**Two-Column Layout**:

**Left Column**:
1. **Network Host Interfaces** (QTableView)
   - Lists local network interfaces (Ethernet, WiFi)
   - Toggle TCP listening on each interface
   - Model: `NetworkInterfacesItemModel`

2. **Network Broadcast Advertisement** (QFrame)
   - Advertise connection URL via UDP broadcast
   - Configure broadcast interface and port (default 8082)
   - Start/stop advertising checkbox

3. **Network Broadcast Discovery** (QGroupBox)
   - Listen for UDP broadcast advertisements
   - Dynamic button list to connect/disconnect from discovered hosts
   - Scroll area populated programmatically

4. **Bluetooth Discovery** (QGroupBox)
   - DISABLED in current UI (`visible=false`)
   - Discover Bluetooth devices with RFCOMM services
   - Connect/disconnect to BT devices

5. **Connected Channels** (QFrame)
   - List of active ALN channels
   - "+" button to manually add channel via `AddChannelDialog`
   - Model: `ConnectionItemModel`

**Right Column**:
1. **Mesh Services** (QScrollArea)
   - Dynamic list of discovered ALN services grouped by node address
   - Click service button to open `PacketSendDialog`
   - Rebuilt on `netStateChanged()` signal

2. **Log Service** (QListView)
   - Displays messages sent to "log" service
   - Shows last 20 messages with source address
   - Model: QStringListModel of `logServiceBufferList`

3. **Status Log** (QListView)
   - Captures qDebug/qInfo/qWarning messages
   - Filter checkboxes: Debug, Info, Error
   - Model: QStringListModel of `debugBufferList`

### Key UI Patterns

**Dynamic Button Groups**:
- `connectToHostButtonGroup`: Connect/disconnect to discovered hosts
- `serviceButtonGroup`: Interact with discovered services
- `btConnectButtonGroup`: Bluetooth device connections

**Model Updates**:
- Models are recreated (not updated) on data change
- Old selection models explicitly deleted to avoid leaks
- Pattern: `ui->view->setModel(new Model(data, this))`

**Signal/Slot Connections**:
- Uses Qt 4 string-based SIGNAL/SLOT macros
- Queued connections for thread safety (`Qt::QueuedConnection`)

### Dialogs

**AddChannelDialog** (`addchanneldialog.{h,cpp}`):
- Simple text input for connection URL
- Format: `[scheme]://host:port[/maln-address]`
- Schemes: tcp, tls, tcp+maln, tls+maln
- Emits: `connectRequest(QString url)`

**PacketSendDialog** (`packetsenddialog.{h,cpp}`):
- Send packet to specific node/service
- Fields: Destination address, Service name, Data
- Registers context handler for echo responses
- Shows response label for received replies

---

## Build System

**Build Tool**: Qt Project (`.pro` file)
**Qt Modules**: core, gui, network, bluetooth, testlib, widgets
**C++ Standard**: C++17
**Config**: `testcase no_testcase_installs` (unit test mode enabled)

**Key Dependencies**:
- Qt 5.x or 6.x (determined by QT_MAJOR_VERSION)
- QTcpServer, QTcpSocket, QSslSocket
- QUdpSocket (for broadcast discovery)
- QBluetoothDeviceDiscoveryAgent, QBluetoothSocket

**Platform Targets**:
- macOS, Linux, QNX (via platform-specific install paths)

**Build Outputs**:
- Executable: `aln-qt` or platform-specific name
- Translations: `aln-qt_en_US.ts` (embedded)
- Resources: `aln-qt.qrc` (empty in current version)

---

## Important Implementation Details

### Thread Safety
- `Router` uses `QMutex` (`mMutex`) to protect shared data structures
- `QMutexLocker` RAII pattern for automatic unlock
- Packet handlers invoked via queued connections to main thread

### Memory Management
- Qt parent-child ownership model (QObject hierarchy)
- Packets allocated on heap, deleted after processing
- Manual cleanup required for dynamically created UI widgets

### Service Registration
Built-in services registered in `MainWindow::init()`:
1. **"log"**: Displays packet data in Log Service view
2. **"echo"**: Echoes packet back to sender (requires context ID)

### Network Discovery Protocol
1. TCP server listens on interface:port
2. AdvertiserThread broadcasts connection URL via UDP every 100ms
3. Other nodes listen on UDP port, populate discovery list
4. User clicks "connect" to establish TCP channel

### Self-Test
`MainWindow::selfTest()` creates 3-node mesh:
- MainWindow router <-> testRouter1 <-> testRouter2
- Uses `LocalChannel` for in-memory testing
- Verifies packet serialization and multi-hop routing

### URL Schemes
- `tcp://host:port` - Plain TCP
- `tls://host:port` - TLS/SSL TCP
- `tcp+maln://host:port/address` - TCP with initial routing packet to specific ALN address
- `tls+maln://host:port/address` - TLS with initial routing

---

## Code Quality Notes

### Strengths
- Clean separation of protocol (aln/) and UI layers
- Abstract Channel interface allows multiple transport types
- Distance-vector routing with service discovery works for small meshes
- Hamming encoding provides basic error correction

### Areas for Improvement
- **No unit tests** (testlib enabled but no test files present)
- **Memory leaks**: Model recreation pattern leaks old models
- **Qt 4 style**: Uses old SIGNAL/SLOT macros instead of function pointers
- **Hardcoded values**: Magic numbers (MAX_DATA_SIZE=1024, port defaults)
- **Limited error handling**: Many `TODO` comments for error cases
- **No encryption**: Packets transmitted in plaintext (except TLS transport)
- **Bluetooth disabled**: Code present but UI hidden and untested
- **Routing limitations**: No route cost metrics beyond hop count
- **Service capacity unused**: `NodeCapacity` tracked but not used for load balancing
- **No persistent state**: All routing/service tables lost on restart

---

## Key TODOs in Code

From source comments:
1. `router.cpp:65` - Detect and broadcast route failures
2. `mainwindow.h:42` - Client socket management
3. `mainwindow.cpp:320` - Handle various URL schemes (only tcp/tls implemented)
4. `tcpchannel.cpp` - Better socket error handling
5. `router.cpp:249` - lastSeen timestamp tracking (commented out)
6. `parser.h:17` - Rename to Ax25FrameReader
7. Bluetooth discovery UI hidden but code present

---

## Refactoring Preparation

### UI Modernization Candidates
1. **Replace string-based signals/slots** with typed connections
2. **Use QML** for dynamic service/discovery lists instead of programmatic widgets
3. **Model updates** instead of recreation (QAbstractItemModel proper implementation)
4. **Separate concerns**: Move network logic out of MainWindow
5. **State management**: Introduce proper state machine for connection lifecycle
6. **Async operations**: Use QtConcurrent for blocking operations

### Architecture Improvements
1. **Dependency injection**: Pass Router as dependency instead of member variable
2. **Service interface**: Abstract PacketHandler into plugin system
3. **Configuration**: External config file instead of hardcoded values
4. **Logging**: Proper logging framework instead of qDebug macros
5. **Testing**: Add unit tests for Packet, Router, Channel implementations

### Protocol Enhancements
1. **Security**: Packet signing and encryption
2. **Reliability**: ACK/NACK mechanism for ackBlock field
3. **QoS**: Priority queuing for packets
4. **Routing**: Link quality metrics beyond hop count
5. **Discovery**: mDNS/Bonjour as alternative to UDP broadcast

---

## Quick Reference: File Purpose Matrix

| File | Purpose | Key Classes/Functions |
|------|---------|----------------------|
| `aln/packet.{h,cpp}` | Packet structure, serialization | `Packet`, `CFHamEncode/Decode` |
| `aln/router.{h,cpp}` | Routing, service discovery | `Router`, `RemoteNodeInfo`, `ServiceNodeInfo` |
| `aln/channel.h` | Transport abstraction | `Channel` (abstract) |
| `aln/tcpchannel.{h,cpp}` | TCP transport | `TcpChannel` |
| `aln/localchannel.{h,cpp}` | Test transport | `LocalChannel` |
| `aln/parser.{h,cpp}` | KISS frame parsing | `Parser` |
| `aln/frame.{h,cpp}` | KISS framing | `toFrameBuffer()` |
| `aln/alntypes.{h,cpp}` | Type utilities | `INT*U`, `readINT*`, `writeINT*` |
| `mainwindow.{h,cpp,ui}` | Main UI logic | `MainWindow` |
| `connectionitemmodel.{h,cpp}` | Channel list model | `ConnectionItem`, `ConnectionItemModel` |
| `networkinterfacesitemmodel.{h,cpp}` | Interface list model | `NetworkInterfaceItem`, `NetworkInterfacesItemModel` |
| `advertiserthread.{h,cpp}` | UDP broadcaster | `AdvertiserThread` |
| `addchanneldialog.{h,cpp}` | Manual connection dialog | `AddChannelDialog` |
| `packetsenddialog.{h,cpp}` | Send packet dialog | `PacketSendDialog` |
| `main.cpp` | Entry point | `main()`, `myMessageOutput()` |

---

## Typical Workflows

### Add New Transport Type
1. Subclass `Channel` in `aln/`
2. Implement `send()`, `listen()`, `disconnect()`
3. Emit `packetReceived()` when frames arrive
4. Emit `closing()` on disconnect
5. Parse frames using `Parser` class
6. Add UI elements to `mainwindow.ui` for configuration
7. Add connection logic in `MainWindow::onConnectRequest()`

### Add New Service
1. Create `PacketHandler` subclass
2. Implement `onPacket(Packet*)` method
3. Register with router: `router->registerService(name, handler)`
4. Service automatically advertised to mesh
5. Optionally add UI in mainwindow for local service interaction

### Debug Packet Flow
1. Enable Debug checkbox in Status Log
2. Watch `Router::onPacket()` qDebug output
3. Check packet `srcAddress`, `destAddress`, `srv` fields
4. Verify routing table with NET_QUERY packets
5. Use LocalChannel self-test for isolated testing

---

## Contact Points for AI Sessions

**Starting Points**:
- `main.cpp:27` - Application initialization
- `mainwindow.cpp:51` - UI setup and router creation
- `router.cpp:331` - Channel lifecycle
- `packet.cpp:56` - Packet constructors

**Extension Points**:
- `router.cpp:369` - Service registration
- `mainwindow.cpp:255` - Add custom channel types
- `packet.h:33` - Packet structure modifications
- `router.h:36` - Custom packet handlers

**Critical Invariants**:
- Packets must be heap-allocated (deleted by router or handler)
- Router must be created before channels
- Services registered before netStateChanged signal expected
- Frame delimiter (0xC0) never appears unescaped in packet data

---

## Summary for AI Context

This is a **working prototype** of an ALN mesh network tool. The protocol layer (`aln/`) is reasonably complete with distance-vector routing and service discovery. The UI (`mainwindow.*`) is functional but rough, suitable for testing and visualization. The codebase is ~2000-3000 lines of C++ using Qt 5/6 APIs.

**Before refactoring**, understand:
1. Router is the central hub - all packets flow through it
2. Channels are transport-agnostic - easy to add new types
3. UI dynamically reflects network state via signals/slots
4. Self-test in mainwindow validates core protocol

**When refactoring**, preserve:
1. Clean protocol/UI separation
2. Channel abstraction
3. Service discovery mechanism
4. Packet structure backward compatibility (wire format)

**Primary goal**: Replace rough Qt Widgets UI with modern, maintainable interface while keeping ALN protocol implementation intact.
