# application-layer-network
Source, documentation and examples of ALN packet handlers in multiple programming languages.

An Application Layer Network applies mesh routing to a packetized messaging system as a means to build networked applications without dedicated messaging infrastructure like MQTT. Setup and integration is designed to be trivial with multi-language support and no library or build-system dependencies.

Drawbacks include that the routing table requires memory on each routing node of an ALN.

## Use Cases
An ALN provides message routing to anywhere in a connected network. A connected network is one where enough links exists that a message can be routed from a node to any other node. An ALN provides messaging connectivity within an application without any concern to network topology or even underlying protocols. One technique is to use an ALN as a protocol bridge to make bluetooth peripherals accessible as services in your network application by using a generic ALN Router instance in an Android app.

# Development Status
| Language   | Packet | Parser | Router |  TCP  | WebSocket |
|------------|--------|--------|--------|-------|-----------|
| C99        | Broken | Broken |        |       |           |
| C#         | Broken |        |        |       |           |
| Go         | Done   | Done   | Proto  |  Done | Done      |
| Java       | Done   | Done   |        |       |           |
| NodeJS     | Done   | Done   | Proto  |  Done | Done      |
| JavaScript | Done   | Done   | Proto  |       | Done      |
| Python     | Done   | Done   | Proto  |  Done |           |

Broken - design changed, code is obselete
Proto - some functionality working but module is not incomplete
Done - functional; implements design
