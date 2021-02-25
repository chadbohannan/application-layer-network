# application-layer-network
Source, documentation and examples of ALN packet handlers in multiple programming languages.

An Application Layer Network applies mesh routing to a packetized messaging system as a means to build networked applications without dedicated messaging infrastructure like MQTT. Setup and integration is designed to be trivial with multi-language support and no library or build-system dependencies.

Drawbacks include that the routing table requires memory on each routing node of an ALN.

## Use Cases
An ALN provides message routing to anywhere in a connected network. A connected network is one where enough links exists that a message can be routed from a node to any other node. An ALN provides messaging connectivity within an application without any concern to network topology or even underlying protocols. One technique is to use an ALN as a protocol bridge to make bluetooth peripherals accessible as services in your network application by using a generic ALN Router instance in an Android app.

# Development Status

## [C99](./c99/README.md)
 * Packet implemented but could be more efficient
 * Parser done.

## [C#](./csharp/README.md)
 * Packet done.
 * Parser CRC32 evaluation is incomplete.

## [Go](./go/README.md)
 * Mesh routing working with automated tests and a TCP example.
 * Route failure handling not started.

## [Java](./java/README.md)
 * Packet implemented but depends on java.util.zip for CRC32 computation.
 * Parser done.

 ## [Python 2.7](./python/README.md)
 * Packet done.
 * Parser done.

 