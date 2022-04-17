# application-layer-network
Source, documentation and examples of ALN packet handlers in multiple programming languages.

An Application Layer Network applies mesh routing to a packetized messaging system as a means to build networked applications without dedicated messaging infrastructure like MQTT. Setup and integration is designed to be trivial with multi-language support and no library or build-system dependencies.

## Summary
An ALN provides message routing to anywhere in a connected network. A connected network is one where enough links exists that a message can be routed from a node to any other node. An ALN provides messaging connectivity within an application without any concern to network topology or even underlying protocols.

To join an ALN, a device must:
 * Create a local Router with a unique address (UUID)
 * Connect to remote Router (TCP/WebSocket/etc)
 * Wrap the new connection in a Channel
 * Add the new Channel to the local Router

The ALN protocol we being sharing route information between the local and remote routers. Routers with service handlers configured will advertise accessible services as well as network metrics for the [distance vector routing protocoal](https://en.wikipedia.org/wiki/Distance-vector_routing_protocol).

Any node can advertise services on a network. An advertised service is only a packet handler for a specific serviceID number. It is up to the application developer to determine what behavior any particular serviceID should have and that this behavior is consistant across their application.

A suggested approach is to advertise service as data consumer nodes. For example, in a sensor network a central data-logger node might advertise service 42 for tempurature data. Any tempurature sensing nodes could then push new tempurature measurement data packets to that node when they are ready.


## Use Cases
 Hobby projects with muliple devices (but not too many) are the intended context for this tool.
 
 * Messaging apps
 * Blinky lights
 * Home switches and sensors
 * Digital toasters

## Limitations
Drawbacks include that the routing table requires memory on each routing node of an ALN.

This software is not recommended for large-scale or high-performance applications as it is more about making things **easy** and less about making them **optimal**. 

# Development Status
| Language   | Packet | Parser | Router |  TCP  | WebSocket | Bluetooth |
|------------|--------|--------|--------|-------|-----------|-----------|
| C99        | Broken | Broken |        |       |           |           |
| C#         | Broken |        |        |       |           |           |
| Go         | Done   | Done   | Proto  |  Done | Done      |           |
| Java       | Done   | Done   |        |       |           |           |
| NodeJS     | Done   | Done   | Proto  |  Done | Done      |           |
| JavaScript | Done   | Done   | Proto  |  n/a  | Done      | n/a       |
| Python     | Done   | Done   | Proto  |  Done |           |           |

Broken - design changed, code is obselete
Proto - some functionality working but module is not incomplete
Done - functional; implements design
