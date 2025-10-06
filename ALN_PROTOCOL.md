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

### 4.1 Frame Structure

ALN packets are transmitted using KISS-style byte stuffing for frame delimitation:

```
Frame ::= SEQUENCE OF {
    packet-data    OCTET STRING,  -- Escaped packet bytes
    frame-end      OCTET STRING   -- 0xC0 delimiter
}
```

### 4.2 Escape Sequences

- **Frame End**: `0xC0` - Indicates end of frame
- **Frame Escape**: `0xDB` - Escape character for byte stuffing
- **Escaped Frame End**: `0xDC` - Literal 0xC0 in data
- **Escaped Frame Escape**: `0xDD` - Literal 0xDB in data

### 4.3 Encoding Algorithm

1. For each byte in the packet:
   - If byte = 0xC0, send 0xDB 0xDC
   - If byte = 0xDB, send 0xDB 0xDD
   - Otherwise, send byte as-is
2. Send 0xC0 to mark end of frame

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

### 6.2 Load Balancing

Service selection uses lowest-load-first algorithm:
- Local services are preferred (load = 0 equivalent)
- Remote services ranked by advertised load metric
- Load metrics are implementation-specific capacity indicators

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

For request-response communication:
1. Client generates unique `contextID`
2. Client sends request with `contextID` set
3. Service responds using same `contextID`
4. Client matches responses using `contextID`

### 8.3 Service Multicast

Services can be multicast by:
- Setting `service` field without `destAddr`
- Routers deliver to all known service instances
- Enables service redundancy and load distribution

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

### 11.3 Optional Features

Implementations may optionally support:
- Packet sequence numbering
- Acknowledgment blocks for reliability
- Custom data types
- CRC validation
- Quality of service metrics

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