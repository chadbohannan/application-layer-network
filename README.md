# application-layer-network
Source, documentation and examples of ALN apps in multiple programming languages.

An Application Layer Network applies mesh routing to a packetized messaging system as a means to build networked applications without dedicated messaging infrastructure like MQTT. Setup and integration is intended to be as simple as possible with multi-language support and as few library or build-system dependencies as possible. This is a hobby-hacker app development tool.

## Summary
An ALN provides message routing to anywhere in a connected network. A connected network is one where enough links exists that a message can be routed from a node to any other node. An ALN provides messaging connectivity within an application without any concern to network topology or even underlying protocols.

To join an ALN, a device must:
 * Create a local Router with a unique address (such as UUID)
 * Connect to remote Router via a bytestream (TCP, TLS, WebSocket, Bluetooth, etc)
 * Wrap the new connection in a Channel
 * Add the new Channel to the local Router

The ALN protocol we being sharing route information between the local and remote routers. Routers with service handlers configured will advertise accessible services as well as network metrics for the xdistance vector routing protocol [(wikipedia)](https://en.wikipedia.org/wiki/Distance-vector_routing_protocol).

Any node can advertise services on a network. An advertised service is a packet handler for a specific service name. It is up to the application developer to determine what behavior any particular service should have and that this behavior is consistant across their application.

The suggested approach is to advertise service as data consumer nodes. For example, in a sensor network a central data-logger node might advertise a `'tempurature log'` service for tempurature data. Any tempurature sensing nodes could then push new tempurature measurement data packets to that node when they are ready.

## Use Cases
 Hobby projects with muliple devices (but not too many) are the intended context for this tool.
 
 * Messaging apps
 * Blinky lights
 * Home switches and sensors
 * Digital toasters

## How It Works
Each node in the network manages a set of channels that eacg wrap a byte stream.
Packets are parsed from the streams and routed either to a packet handler registered by the application programmer or acts as an intermediate routing node and retransmits the packet on a channel known to (at least eventually) reach the packet's destination address.

Routes and available service listings are shared by the NET_SERVICE and NET_ROUTE type packets that are handled with network flood semantics. If the local node makes a change to it's routing/service records, the packets are retransmitted on all connected channels except the channel it was received on.

The route and service tables will grow at each network as the size of the network grows. This is a natural effect of a distributed system. Support for disabling routing may be introduced in the future for small-memory systems. 

## Limitations
This software is not recommended for large-scale or high-performance applications as it is more about prototyping ideas **easily** and less about building production applications **optimally**. 

# Development Status
| Language      |  TCP  | TLS   | WebSocket | Bluetooth |
|---------------|-------|-------|-----------|-----------|
| C++/Qt        | Works |       |           |           |
| Go            | Works | Works | Works     |           |
| Java          | Works |       |           |           |
| Java (Android)| Works | Works |           | Works     |
| NodeJS        | Works |       | Works     |           |
| JavaScript    | n/a   | n/a   | Works     | n/a       |
| Python        | Works |       |           | Works     |

