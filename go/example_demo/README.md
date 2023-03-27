## Example Demo
This example is a demonstration of a two-node network within a single program instance.

In this example, the addresses `r1` and `r2` are reserved respectively for the two routers.
The first one is configured as a host by creating TcpChannelHost and configuring it to add new channels to `r1`.

Then a connection is made by opening a sockect with the host and wrapping that socket in a TcpChannel before adding the client socket to `r2`.

`r1` is configugred with a `ping` service handler, and `r2` is configured with a ping response handler.

A service handler differs from response handler in that a services are advertised by its string-typed name and response handlers are accessible by the int-typed contextID. In this example `r2` is used to send a `ping` service packet to `r1` which responds with a packet to the originating node (`r2`) with the contextID field set so as to reach the response handler. Here, `r2` releases the context to free resources as it no longer expects any further response packets from `r1`.

Lastly, when `r2` recieves the response packet it also releases the blocking waitgroup allowing the program to exit and conclude the demo.
