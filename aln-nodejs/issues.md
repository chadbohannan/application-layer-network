# aln-nodejs Deployment Readiness

**Last Updated:** 2025-10-09
**Status:** ⚠️  Near-ready (1 blocker)

## Deployment Checklist

### Core Requirements
- ✅ Package name: `aln-nodejs` (available on npm)
- ✅ Version: 1.0.0 (ready for initial release)
- ✅ License: MIT (declared in package.json)
- ✅ README.md: Complete with examples and API documentation
- ✅ Repository: GitHub URL configured
- ✅ Keywords: Appropriate npm keywords defined
- ✅ Main entry: `lib/aln/router.js` correctly set
- ✅ Tests: 9 passing tests covering core functionality
- ✅ Linting: Standard.js configured and passing

### Protocol Compliance
- ✅ Packet parsing/serialization: Complete
- ✅ KISS framing: Working (TCP)
- ✅ JSON framing: Working (WebSocket)
- ✅ Hamming error correction: Implemented
- ✅ Distance-vector routing: Functional
- ✅ Service discovery: Working with load-based selection
- ✅ Multi-hop routing: Tested (3-router topology)
- ✅ Service multicast: Working

---

## Deployment Blocker

### Missing LICENSE File
**Severity:** 🔴 BLOCKER
**Issue:** Package.json declares MIT license but no LICENSE file exists
**Fix:** Add MIT LICENSE file to aln-nodejs directory

---

## Known Issues (Non-Blocking)

### High Priority

**Split Horizon Missing** (router.js:179)
- Routes advertised back to originating channel
- May cause routing loops in complex topologies
- Fix: Check `RemoteNodeInfo.channel` before relaying routes

**WebSocket Channel Incomplete** (wschannel.js)
- Missing `close()` method
- `onClose` handler is properly wired (lines 28-31)
- Impact: Cannot programmatically close WebSocket channels

### Medium Priority

**Service Removal Relay** (router.js:201-206)
- Service removal (capacity=0) handled locally
- Needs verification that removals propagate to other channels

**Channel.send() Error Handling** (tcpchannel.js:24-27, wschannel.js:40-42)
- No error return value (spec requires it)
- Fix: Wrap send in try-catch, return error status

### Low Priority (Code Quality)

**Suspicious Route Logic** (router.js:171-173)
- Route removal check inside "add route" block
- May need refactoring for clarity

**No Error Packet Generation**
- Spec defines `netError` packets but none generated
- Local error strings used instead (sufficient for now)

**Data Size Field** (packet.js:82)
- `packet.sz` not set during serialization
- Minor inconsistency, no functional impact

---

## Future Enhancements (Cross-Implementation)

These are defined in ALN_PROTOCOL.md but unimplemented across Go, Python, and Node.js:

**Route Expiration** (router.js:27)
- `lastSeen` tracked but routes never expire
- Spec requires timeout-based expiration
- Affects all three implementations

**Periodic Advertisements** (router.js)
- Only triggered updates implemented
- Spec requires periodic full table dumps
- Affects all three implementations

**Service Capacity Aging** (router.js:276-278)
- Service `lastSeen` tracked but never filtered
- Stale services persist indefinitely
- Affects all three implementations

**CRC32 Validation** (packet.js:46)
- Optional spec feature
- Go/Python calculate but don't verify
- Node.js has no CRC support
- Only needed for unreliable transports (radio, serial)

---

## Historical Bug Fixes (Completed)

### Critical Bugs Fixed (2025-10-06)

**1. Packet DataType Parsing** (packet.js:40)
- Issue: Checked `this.typ` instead of `this.cf` for CF_DATATYPE flag
- Fixed: Corrected to `if (this.cf & CF_DATATYPE)`
- Verified with Go server interop testing

**2. Frame Escape Detection** (parser.js:62)
- Issue: Array comparison `b === [Esc]` always false
- Fixed: Changed to `b === Esc`
- KISS framing now works correctly

**3. Hamming Error Correction** (packet.js:31, 110-132)
- Issue: `cfHamDecode()` existed but was never called
- Fixed: Enabled Hamming(15,11) decode on control flags
- Single-bit error correction now active

**4. Router Property Case Mismatch** (router.js:174-176)
- Issue: Used PascalCase (`NextHop`, `Channel`, `Cost`)
- Fixed: Corrected to camelCase (`nextHop`, `channel`, `cost`)
- Route updates now work properly

**5. WebSocket onClose Handler** (wschannel.js:28-31)
- Issue: No close event handling
- Fixed: Added `ws.on('close')` handler
- Channels now clean up on disconnect

**6. Service Removal** (router.js:201-206)
- Issue: Service removal (capacity=0) not handled
- Fixed: Service entries now deleted when capacity=0
- Still needs verification of propagation

---

## Compliance Summary

**Overall Status:** ~90% protocol compliance
- Core routing: ✅ Fully functional
- Service discovery: ✅ Working with load-based selection
- Multi-hop routing: ✅ Tested (3-router topology)
- Channel cleanup: ✅ Route withdrawal on channel close
- Error correction: ✅ Hamming codes implemented
- Tests: ✅ 9 passing (packet, router, utils, multi-hop, multicast)

**Critical bugs:** 0 remaining (6 of 6 fixed)
**High-priority issues:** 2 non-blocking (split horizon, WebSocket close method)
**Cross-implementation gaps:** 4 future enhancements (all implementations affected)
