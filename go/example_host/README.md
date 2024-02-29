## Exmple Host in Go
This example host implementation starts by initializing a new Router with address `go-host-3f99ea`.
It is best to keep node addresses unique; if two nodes on the network have the same address the network will not reliably route packets.

This example then opens a tcp host socket on `localhost:8000`.
If you would like to make this host node accessible from other machines on your LAN, replace `localhost` with the IP address of your ethernet/wifi adapter.

Next the `ping` service handler is defined and registered with the router. 
The router will advertise the service on connected channels automatically.

Lastly the example waits for a termincation signal from the shell environment.
At this point the concurrency features of Go are being used to listen for new connections and advertise the `ping` service.
Now it's time to connect using a client.