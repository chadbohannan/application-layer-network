## Exmple Client in Go
This example client implementation starts by initializing a new Router with address `go-client-da01b5`.
It is best to keep node addresses unique; if two nodes on the network have the same address the network will not reliably route packets.

This example then attempts to connnect a tcp socket on `localhost:8081`.

Next the `ping` response handler is defined and registered with the router. 
The router will NOT advertise the handler and only packets sent to `go-client-da01b5` with the correct ctx ID will be routed to that handler.

Then this program sends a packet with DestAddr, Service and ContextID values.

Lastly the example waits for a termincation signal from the shell environment.

Now it's time to connect using a client.

### As an Exercise
In this example a client application is implemented to send a single packet to a specific service.

As a handy feature, the Router can multicast a packet with a Service specfied but no DestAddr. In this case each instance of a service on the network will be sent a copy of the outbound packet.

Try adding a `ping` handler to this client and removing/commenting the the `wg.Done()` line to prevent the client from exiting.
Then open multiple CLI shells and run several clients at once, creating a larger network of nodes each with a `ping` handler.
You should see each node in the network, clients and servers, respond to the ping packet.
