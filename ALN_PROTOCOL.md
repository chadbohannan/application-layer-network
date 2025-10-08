# Application Layer Network (ALN) Protocol Specification

## 1. Introduction

The Application Layer Network (ALN) protocol is a mesh networking protocol designed for distributed service discovery and communication. ALN enables dynamic routing, service advertisement, and load balancing across a network of nodes.

### 1.1 Protocol Objectives

- **Service Discovery**: Automatic advertisement and discovery of network services
- **Dynamic Routing**: Distance-vector routing with automatic route updates
- **Load Balancing**: Service selection based on capacity metrics
- **Mesh Networking**: Multi-hop routing between arbitrary network nodes
- **Transport Agnostic**: Protocol operates over various transport mechanisms

## 2. Protocol Architecture

### 2.1 Network Model

ALN operates as an overlay network where:
- **Nodes** are identified by unique addresses
- **Channels** provide transport connectivity between nodes
- **Services** are named endpoints that handle application requests
- **Routers** manage packet forwarding and maintain network state

### 2.2 Address Space

```asn1
AddressType ::= UTF8String (SIZE(1..255))
```

Addresses are variable-length UTF-8 strings uniquely identifying network nodes.

## 3. Packet Format

### 3.1 Packet Structure

```asn1
ALNPacket ::= SEQUENCE {
    controlFlags    ControlFlags,
    netState        NetworkState OPTIONAL,
    service         UTF8String (SIZE(1..255)) OPTIONAL,
    srcAddr         AddressType OPTIONAL,
    destAddr        AddressType OPTIONAL,
    nextAddr        AddressType OPTIONAL,
    seqNum          INTEGER (0..65535) OPTIONAL,
    ackBlock        INTEGER (0..4294967295) OPTIONAL,
    contextID       INTEGER (0..65535) OPTIONAL,
    dataType        INTEGER (0..255) OPTIONAL,
    dataSize        INTEGER (0..65535) OPTIONAL,
    data            OCTET STRING (SIZE(0..4096)) OPTIONAL,
    crcSum          INTEGER (0..4294967295) OPTIONAL
}
```

### 3.2 Control Flags

The control flags field uses a 16-bit value with Hamming error correction encoding. The upper 5 bits contain Hamming parity bits, while the lower 11 bits specify which optional fields are present:

```asn1
ControlFlags ::= BIT STRING (SIZE(16))

-- Hamming parity bits (bits 15-11)
CF-HAMMING1  ::= BIT STRING (SIZE(1)) -- bit 15
CF-HAMMING2  ::= BIT STRING (SIZE(1)) -- bit 14
CF-HAMMING3  ::= BIT STRING (SIZE(1)) -- bit 13
CF-HAMMING4  ::= BIT STRING (SIZE(1)) -- bit 12
CF-HAMMING5  ::= BIT STRING (SIZE(1)) -- bit 11

-- Field presence flags (bits 10-0)
CF-NETSTATE  ::= BIT STRING (SIZE(1)) -- bit 10
CF-SERVICE   ::= BIT STRING (SIZE(1)) -- bit 9
CF-SRCADDR   ::= BIT STRING (SIZE(1)) -- bit 8
CF-DESTADDR  ::= BIT STRING (SIZE(1)) -- bit 7
CF-NEXTADDR  ::= BIT STRING (SIZE(1)) -- bit 6
CF-SEQNUM    ::= BIT STRING (SIZE(1)) -- bit 5
CF-ACKBLOCK  ::= BIT STRING (SIZE(1)) -- bit 4
CF-CONTEXTID ::= BIT STRING (SIZE(1)) -- bit 3
CF-DATATYPE  ::= BIT STRING (SIZE(1)) -- bit 2
CF-DATA      ::= BIT STRING (SIZE(1)) -- bit 1
CF-CRC       ::= BIT STRING (SIZE(1)) -- bit 0
```

### 3.3 Network State Types

```asn1
NetworkState ::= ENUMERATED {
    netRoute(1),     -- Route advertisement packet
    netService(2),   -- Service advertisement packet
    netQuery(3),     -- Network state query packet
    netError(255)    -- Error notification packet
}
```

### 3.4 Wire Format

Packets are serialized as binary data with the following encoding:

1. **Multi-byte integers** are encoded in big-endian format
2. **Strings** are length-prefixed with a single byte indicating length
3. **Optional fields** are included only when corresponding control flag is set
4. **Field order** follows the sequence defined in the packet structure

#### Field Sizes
- Control Flags: 2 bytes (16-bit unsigned integer)
- Network State: 1 byte (8-bit unsigned integer)
- Service: 1 byte length + string data (max 255 bytes)
- Address fields: 1 byte length + string data (max 255 bytes each)
- Sequence Number: 2 bytes (16-bit unsigned integer)
- Acknowledgment Block: 4 bytes (32-bit unsigned integer)
- Context ID: 2 bytes (16-bit unsigned integer)
- Data Type: 1 byte (8-bit unsigned integer)
- Data Size: 2 bytes (16-bit unsigned integer)
- Data: Variable length (max 4096 bytes)
- CRC Sum: 4 bytes (32-bit unsigned integer)

## 4. Frame Layer

The frame layer provides packet delimitation over transport channels. ALN supports two framing methods depending on the transport type.

### 4.1 Binary Frame Structure (KISS Framing)

For stream-based transports (TCP, serial, etc.), ALN packets are transmitted using KISS-style byte stuffing for frame delimitation:

```
Frame ::= SEQUENCE OF {
    packet-data    OCTET STRING,  -- Escaped packet bytes
    frame-end      OCTET STRING   -- 0xC0 delimiter
}
```

**Escape Sequences:**
- **Frame End**: `0xC0` - Indicates end of frame
- **Frame Escape**: `0xDB` - Escape character for byte stuffing
- **Escaped Frame End**: `0xDC` - Literal 0xC0 in data
- **Escaped Frame Escape**: `0xDD` - Literal 0xDB in data

**Encoding Algorithm:**
1. For each byte in the packet:
   - If byte = 0xC0, send 0xDB 0xDC
   - If byte = 0xDB, send 0xDB 0xDD
   - Otherwise, send byte as-is
2. Send 0xC0 to mark end of frame

### 4.2 WebSocket Frame Structure (JSON Encoding)

For WebSocket transports, ALN packets MAY use JSON encoding instead of binary KISS framing:

**Rationale:**
- WebSocket provides built-in message framing
- No need for byte stuffing or special delimiters
- JSON encoding simplifies debugging and cross-language compatibility
- Human-readable packet inspection

**JSON Packet Format:**
```json
{
  "cf": <uint16>,           // Control flags (optional)
  "net": <uint8>,           // Network state type (optional)
  "srv": "<string>",        // Service name (optional)
  "src": "<string>",        // Source address (optional)
  "dst": "<string>",        // Destination address (optional)
  "nxt": "<string>",        // Next hop address (optional)
  "seq": <uint16>,          // Sequence number (optional)
  "ack": <uint32>,          // Acknowledgment block (optional)
  "ctx": <uint16>,          // Context ID (optional)
  "typ": <uint8>,           // Data type (optional)
  "data": "<base64>",       // Packet data as base64 (optional)
  "data_sz": <uint16>       // Data size in bytes (optional)
}
```

**Field Mapping:**
- All packet fields map directly to JSON properties
- Binary data is base64-encoded
- Null/undefined fields are omitted (sparse encoding)
- `data_sz` provides the original binary data length

**WebSocket Channel Implementation:**
- Send: Serialize packet to JSON, send as WebSocket text message
- Receive: Parse JSON message, decode base64 data, construct packet
- The WebSocket frame itself provides message boundaries

**Transport Selection:**
- TCP/Serial channels: MUST use binary KISS framing (Section 4.1)
- WebSocket channels: SHOULD use JSON encoding (Section 4.2)
- WebSocket channels MAY use binary KISS framing for compatibility
- Implementations MUST document which framing method is used per transport

## 5. Routing Protocol

### 5.1 Distance Vector Algorithm

ALN uses a distance-vector routing protocol with the following characteristics:

- **Metric**: Hop count (16-bit unsigned integer)
- **Update Mechanism**: Periodic and triggered updates
- **Loop Prevention**: Split horizon with poison reverse

### 5.2 Route Advertisement Format

Route advertisements use `netRoute` packets with the following data payload:

```
RouteAdvertisement ::= SEQUENCE {
    destAddrLength  INTEGER (1..255),
    destAddr        OCTET STRING,
    cost           INTEGER (0..65535)
}
```

Where:
- `destAddrLength`: Length of destination address
- `destAddr`: Destination address being advertised
- `cost`: Hop count to destination (0 = route withdrawal)

### 5.3 Routing Behavior

#### Route Learning
- Nodes advertise themselves with cost 1
- Received routes are incremented by 1 before re-advertisement
- Better routes (lower cost) replace existing routes
- Routes expire if not refreshed within timeout period

#### Route Propagation
- New routes trigger immediate advertisements
- Route withdrawals (cost 0) are propagated immediately
- Periodic full routing table advertisements

## 6. Service Discovery Protocol

### 6.1 Service Advertisement Format

Service advertisements use `netService` packets with the following data payload:

```
ServiceAdvertisement ::= SEQUENCE {
    hostAddrLength  INTEGER (1..255),
    hostAddr        OCTET STRING,
    serviceLength   INTEGER (1..255),
    serviceName     OCTET STRING,
    serviceLoad     INTEGER (0..65535)
}
```

Where:
- `hostAddrLength`: Length of host address
- `hostAddr`: Address of node providing service
- `serviceLength`: Length of service name
- `serviceName`: Name of advertised service
- `serviceLoad`: Current load metric (0 = service removal)

### 6.2 Service Load Metrics

The `serviceLoad` field is a 16-bit unsigned integer representing the current capacity or utilization of a service instance:

**Load Value Semantics:**
- `0`: Service removal - indicates the service is no longer available at this address
- `1-65535`: Active service with current load/capacity metric

**Load Metric Interpretation:**
The load value is implementation-specific but should follow these guidelines:
- **Lower values indicate better availability** (less loaded, more capacity)
- **Higher values indicate higher utilization** (more loaded, less capacity)
- Common implementations:
  - Active connection count
  - CPU/memory utilization percentage (0-100 scale)
  - Queue depth or pending request count
  - Inverse capacity measure (65535 = fully loaded)

### 6.3 Service Selection and Load Balancing

When multiple instances of a service are available, nodes select the optimal instance using the following algorithm:

**Selection Priority:**
1. **Local Service First**: If the service is registered locally, always use it (no network hop)
2. **Lowest Remote Load**: Select the remote service instance with the lowest `serviceLoad` value
3. **No Service**: If no instances are available, routing fails

**Load-Based Selection Algorithm:**
```
function SelectService(serviceName):
    if localServiceExists(serviceName):
        return localAddress

    minLoad = ∞
    selectedAddress = null

    for each (address, load) in remoteServices[serviceName]:
        if load > 0 and load < minLoad:
            minLoad = load
            selectedAddress = address

    return selectedAddress
```

**Load Update Propagation:**
- Service load changes are propagated immediately as triggered updates
- Each router relays service advertisements to all other channels
- Duplicate advertisements (same address, service, load) are suppressed to prevent loops
- Service removal (`load=0`) is propagated to all channels immediately

### 6.4 Service Lifecycle Management

**Service Registration:**
1. Node registers local service handler
2. Node sends `netService` packet with `serviceLoad=1` (or actual load) to all channels
3. Neighbors update service tables and relay to their channels

**Service Update:**
1. Node measures current service load
2. If load changes significantly, send updated `netService` packet
3. Update frequency is implementation-defined (recommend throttling to avoid floods)

**Service Removal:**
1. Node unregisters service handler
2. Node sends `netService` packet with `serviceLoad=0` to all channels
3. Neighbors remove service from tables and relay removal to their channels
4. Service entries are also removed when their host route expires or is withdrawn

## 7. Network State Management

### 7.1 Network Queries

Nodes can request full network state using `netQuery` packets:
- Contains no data payload
- Recipients respond with complete routing and service tables
- Used for initial network discovery and synchronization

### 7.2 State Synchronization

Network state is synchronized through:
- **Triggered Updates**: Immediate propagation of changes
- **Periodic Updates**: Full table advertisements at intervals
- **Query Response**: Complete state dump on request

## 8. Application Data Transport

### 8.1 Service Communication

Application data is transported using standard ALN packets with:
- `destAddr`: Target service node address
- `service`: Target service name
- `data`: Application payload

### 8.2 Request-Response Pattern

The `contextID` field enables request-response communication by correlating responses with their originating requests. This is an **implementation pattern** - the protocol only defines the field; applications must manage the mapping.

**Protocol Flow:**
1. Client generates unique `contextID` (non-zero, 16-bit unsigned integer)
2. Client registers a response handler mapped to this `contextID`
3. Client sends request packet with `contextID` field set
4. Service receives request, processes it, and sends response(s) with the same `contextID`
5. Client receives response(s), looks up handler by `contextID`, and invokes it
6. **Client is responsible for cleanup** - must explicitly release the context when done

**Implementation Pattern (Pseudocode):**
```
// Client side - sending request
ctxID = generateUniqueContextID()
registerContextHandler(ctxID, (responsePacket) => {
    // Handle response
    console.log("Received:", responsePacket.data)

    // Clean up when done (user's responsibility)
    // May receive multiple responses before releasing
    if (isDone) {
        releaseContext(ctxID)
    }
})

sendPacket({
    srv: "my-service",
    ctx: ctxID,
    data: "request payload"
})

// Service side - responding
serviceHandler = (requestPacket) => {
    // Process request...

    // Send response(s) with same contextID
    sendPacket({
        dst: requestPacket.src,  // Reply to sender
        ctx: requestPacket.ctx,  // Use same context
        data: "response payload"
    })

    // May send multiple responses for same context
    sendPacket({
        dst: requestPacket.src,
        ctx: requestPacket.ctx,
        data: "additional data"
    })
}
```

**Important Considerations:**
- **Multiple Responses**: A service may send multiple packets for a single request (streaming, pagination, progress updates). The client must track completion and release the context when appropriate.
- **Cleanup**: Contexts consume memory. Clients MUST release contexts when done to prevent memory leaks.
- **Timeouts**: Implementations should consider timeout mechanisms for contexts that never receive responses.
- **Context ID 0**: Reserved. Valid context IDs are 1-65535.
- **Uniqueness**: Context IDs must be unique per client node at any given time. Reuse after release is acceptable.

### 8.3 Service Multicast

Service multicast enables sending a single packet to all instances of a service across the network.

**Multicast Packet Format:**
- `service`: Target service name (required)
- `destAddr`: Empty/unset (triggers multicast behavior)
- `data`: Application payload
- Other fields as needed (srcAddr, contextID, etc.)

**Multicast Delivery Behavior:**
1. If local service exists, deliver to local handler first
2. For each remote service instance:
   - Create a copy of the packet
   - Set `destAddr` to the remote service's address
   - Route the copy to that destination
3. All instances receive the same payload independently

**Use Cases:**
- Service redundancy: Send to all instances for fault tolerance
- State synchronization: Broadcast state updates to all service nodes
- Load distribution: Let all instances process the request
- Discovery: Query all instances for capabilities

**Important Considerations:**
- Each service instance receives an independent copy
- No ordering guarantees between instances
- Responses (if any) must be handled per-instance using `contextID`
- Multicast to non-existent service fails silently (no instances to deliver to)

## 9. Error Handling

### 9.1 Error Reporting

Network errors are reported using `netError` packets:
- `data` field contains error message as UTF-8 text
- Errors are logged but do not trigger automatic responses

### 9.2 Error Correction

The protocol includes error detection and correction:
- **Hamming Codes**: Single-bit error correction in control flags
- **CRC32**: Packet integrity verification when CRC flag is set
- **Frame Validation**: SLIP framing detects transmission errors

## 10. Security Considerations

### 10.1 Trust Model

ALN operates under a trusted network model:
- All nodes are assumed to be cooperative
- No authentication or encryption at protocol level
- Security must be provided by underlying transport

### 10.2 Denial of Service

Potential DoS vectors include:
- Route table poisoning through false advertisements
- Service flooding through excessive advertisements
- Resource exhaustion through large packet floods

Implementations should include rate limiting and validation mechanisms.

## 11. Implementation Guidelines

### 11.1 Channel Interface

Transport implementations must provide:
```
interface Channel {
    Send(packet) -> error
    Receive(callback)
    Close()
    OnClose(callback)
}
```

### 11.2 Required Behaviors

Conforming implementations must:
- Implement complete packet parsing and serialization
- Support all network state packet types
- Maintain routing and service tables
- Provide service registration and discovery APIs
- Handle frame-level escaping correctly
- Implement service load-based selection (prefer local, then lowest load)
- Handle service removal (load=0) correctly
- Suppress duplicate service advertisements to prevent loops

### 11.3 Optional Features

Implementations may optionally support:
- Packet sequence numbering
- Acknowledgment blocks for reliability
- Custom data types
- CRC validation
- Quality of service metrics
- Service capacity change notifications/callbacks
- Dynamic load measurement and reporting
- Service load update throttling

## 12. Constants and Limits

### 12.1 Size Limits
- Maximum packet size: 5120 bytes
- Maximum data payload: 4096 bytes
- Maximum header size: 1024 bytes
- Maximum string fields: 255 bytes each

### 12.2 Magic Numbers
- Frame end delimiter: 0xC0
- Frame escape character: 0xDB
- Escaped frame end: 0xDC
- Escaped frame escape: 0xDD

### 12.3 Protocol Version

This specification describes ALN Protocol version 1.0. Future versions should maintain backward compatibility with the packet format and core routing behaviors described herein.