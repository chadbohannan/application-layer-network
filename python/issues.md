# Python ALN Implementation Issues

## Overview

This document outlines discrepancies between the Python ALN implementation and the protocol specification in `go/protocol_spec.md`. These issues were identified through analysis of `python/src/alnmeshpy/router.py` and `python/src/alnmeshpy/packet.py` against the reverse-engineered Go protocol specification.

## Critical Issues

### 1. Service Load vs Service Capacity Semantic Mismatch
**Files**: `router.py:31`, `router.py:141`
**Specification Reference**: `protocol_spec.md:205`

- **Issue**: Python uses "serviceCapacity" terminology while spec defines "serviceLoad"
- **Impact**: Semantic confusion - load typically increases with usage, capacity decreases
- **Affected Functions**: `makeNetworkServiceSharePacket()`, `parseNetworkServiceSharePacket()`, service capacity maps
- **Fix Required**: Rename variables and update logic to match load-based semantics

### 2. Missing Network Error Support
**Files**: `packet.py:153-155`
**Specification Reference**: `protocol_spec.md:90`

- **Issue**: Python lacks `NET_ERROR = 255` constant and error handling
- **Impact**: Cannot generate or process error packets as defined in protocol
- **Missing**: Error packet creation, error message handling in router
- **Fix Required**: Add NET_ERROR constant and implement error packet support

### 3. Packet Size Limit Violation
**Files**: `packet.py:150`
**Specification Reference**: `protocol_spec.md:323`

- **Issue**: Python allows 65535 byte data payload, spec limits to 4096 bytes
- **Impact**: Can create oversized packets that violate protocol specification
- **Fix Required**: Change `MAX_DATA_SIZE = 0xFFFF` to `MAX_DATA_SIZE = 4096`

## High Priority Bugs

### 4. Data Type Field Serialization Bug
**File**: `packet.py:254`
**Issue**: `toPacketBuffer()` writes `self.netState` instead of `self.dataType`
```python
# Current (incorrect)
packetBuffer.append(self.netState)
# Should be
packetBuffer.append(self.dataType)
```

### 5. ACK Block Field Size Mismatch
**File**: `packet.py:320`
**Specification Reference**: `protocol_spec.md:109`

- **Issue**: Uses `readINT16U()` for 32-bit ACK block field
- **Impact**: Cannot properly parse 32-bit acknowledgment values
- **Fix**: Change to `readINT32U(ackBytes)`

### 6. JSON Parsing Assignment Error
**File**: `packet.py:366`
**Issue**: Assigns context ID value to control flags
```python
# Current (incorrect)
p.controlFlags = obj["ctx"]
# Should be
p.contextID = obj["ctx"]
```

## Medium Priority Issues

### 7. Incorrect Error Messages
**File**: `router.py:43`
**Issue**: `parseNetworkServiceSharePacket()` returns error mentioning "NET_ROUTE" instead of "NET_SERVICE"

### 8. Missing CRC Validation
**Specification Reference**: `protocol_spec.md:266`
**Issue**: CRC computation exists but validation logic not implemented
- CRC calculation function present in `packet.py:42`
- No validation of received packets against CRC field

## Incomplete Features (TODOs)

### 9. Route Expiration
**Files**: `router.py:293`, `router.py:307`
**Issue**: Comments indicate missing expiration filtering for routes and services
- No timestamp validation for route freshness
- Expired routes not automatically removed

### 10. Service Capacity Sorting
**File**: `router.py:306`
**Issue**: TODO comment indicates services should be sorted by decreasing capacity
- Current implementation sends services in arbitrary order
- Load balancing effectiveness reduced

## Compatibility Concerns

### 11. Control Flag Hamming Code Implementation
**Files**: `packet.py:67-98`
**Issue**: Hamming encode/decode functions exist but bit position alignment with specification unclear
- May cause interoperability issues with Go implementation
- Requires validation against spec bit mappings (protocol_spec.md:62-67)

## Recommendations

1. **Immediate**: Fix critical bugs (issues #4, #5, #6) to prevent packet corruption
2. **High Priority**: Implement NET_ERROR support and fix packet size limits
3. **Medium Priority**: Address semantic mismatch between load/capacity concepts
4. **Long Term**: Implement missing features (route expiration, CRC validation)

## Impact Assessment

- **Interoperability**: High risk due to packet format bugs and missing error handling
- **Protocol Compliance**: Medium risk due to size limits and semantic mismatches
- **Reliability**: Medium risk due to missing validation and expiration logic
- **Performance**: Low risk, mostly affects load balancing efficiency