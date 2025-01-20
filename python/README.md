# Application Layer Network - Python 3

`alnmeshpy` is a Python implementation of Application Layer Network (ALN),
an amature mesh networking protocol for home lab networking designed for 
hacking prototype projects that benefit from the simplicity gained from
completely ignoring security as a concern. Don't use this for production.

Install with `--no-deps` unless you need Bluetooth support (very experimental) which use `bleak` and `aiofiles`.

```sh
pip install --no-deps -i https://test.pypi.org/simple/ alnmeshpy
```

The example app demonstrates a 3-node topology
 * Node 1 hosts TCP connections and demonstrates mesh networking
 * Node 2 connects to Node 1 and provides a 'ping' service
 * Node 3 connects to Node 1 and sends packets to 'ping' service

## Running example
### Node 1
```py
import selectors, signal
from alnmeshpy import TcpHost, Router

def main():
    sel = selectors.DefaultSelector() # app event loop
    router = Router(sel, "python-host-1") # must be unique
    router.start() # Router is a Thread object

    def on_connect(tcpChannel, addr): # define TCP host listener 
        router.add_channel(tcpChannel) # give all new channels to our single router
    TcpHost().listen(sel, 'localhost', 8081, on_connect)
    
    def signal_handler(signal, frame): # listen for ^C
        router.close()
        sel.close()
    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()

if __name__ == "__main__":
    main()

```

### Node 2
```py
import selectors, signal, socket
from alnmeshpy import TcpChannel, Router,Packet

def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-ping-service-1")
    router.start() # router s a Thread object so it must be started

    def ping_handler(packet):
        print("ping from [{0}]".format(packet.srcAddr))
        router.send(Packet(
            destAddr=packet.srcAddr, # return to sender
            contextID=packet.contextID, # required
            data="pong"
        ))
    router.register_service("ping", ping_handler)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8081)) # hard-coded address for demonstration
    router.add_channel(TcpChannel(sock)) # let the router operate on this channel

    def signal_handler(signal, frame):
        router.close()
        sel.close()
    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()

if __name__ == "__main__":
    main()

```

### Node 3
```py
import selectors, signal, socket, time
from alnmeshpy import TcpChannel, Router,Packet

def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-ping-client-1")
    router.start()

    def on_pong(packet): # define the ping response handler
        print('received:', str(packet.data), 'from:', packet.srcAddr)
        time.sleep(1) # wait a second then ping again
        router.send(Packet(service="ping", contextID=packet.contextID))
    ctxID = router.register_context_handler(on_pong) # set response handler

    # send a packet when a 'ping' service is discovered
    def on_service_discovery(service, capacity, address):
        print("on_service_discovery: {0}:{1}:{2}".format(service, capacity, address))
        if service == "ping":
            print("sending ping with ctxID {0}".format(ctxID))
            router.send(Packet(destAddr=address, service="ping", contextID=ctxID))      
    router.set_on_service_capacity_changed_handler(on_service_discovery)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8081)) # hard-coded address for demonstration

    router.add_channel(TcpChannel(sock)) # let the router operate on this channel

    def signal_handler(signal, frame):
        router.close()
        sel.close()
    signal.signal(signal.SIGINT, signal_handler)   
    signal.pause()

if __name__ == "__main__":
    main()

```

## Integration test
This test is functionally equivalent but uses pipes 
to connect all 3 routers within a single process.
This script serves as a useful integration test when
hacking on the protocol.

```py
import os, selectors, signal
from threading import Lock
from alnmeshpy import LocalChannel, Router,Packet

# this is an integration test that configures 3 nodes
# node 1 advertises a ping service that responds with a 'pong'
# node 2 is an intermediate node that demonstrates multi-hop routing
# node 3 listens for the ping service advertisment and sends a pack to it
# finally node 3 prints the content of the ping packet

def main():
    
    r1a, w1a = os.pipe()
    r2a, w2a = os.pipe()
    ch1a = LocalChannel(r1a, w2a)
    ch2a = LocalChannel(r2a, w1a)

    sel = selectors.DefaultSelector()

    # configure the ping handler on node 1
    def ping_handler(packet):
        print("ping from [{0}]".format(packet.srcAddr))
        router1.send(Packet(
            destAddr=packet.srcAddr,
            contextID=packet.contextID,
            data="pong"
        ))

    router1 = Router(sel, "aln-node-1")
    router1.start()
    router1.register_service("ping", ping_handler)
    
    # node 2 exists to test mesh networking
    router2 = Router(sel, "aln-node-2")
    router2.start()
    
    router1.add_channel(ch1a)
    router2.add_channel(ch2a)
    
    r1b, w1b = os.pipe()
    r2b, w2b = os.pipe()
    ch1b = LocalChannel(r1b, w2b)
    ch2b = LocalChannel(r2b, w1b)

    # node 3 will:
    #  listen for the ping service advertisment advertisement
    #  send a 'ping' packet to the advertising node
    #  print the response to the console
    router3 = Router(sel, "aln-node-3")
    router3.start()

    # set up listener for ping response
    def on_packet(packet):
        print(packet.toJsonString())
        print('unpacked response data:', packet.data)
        print('ctrl-C to quit')
    ctxID = router3.register_context_handler(on_packet)

    # set up a service discovery handler for the "ping" service
    def on_service_discovery(service, capacity, address):
        print("on_service_discovery at aln-node-3: {0}:{1}:{2}".format(service, capacity, address))
        if service == "ping":
            print("sending ping with ctxID {0}".format(ctxID))
            text = "hello world".encode(encoding="utf-8")
            router3.send(Packet(destAddr=address, service="ping", contextID=ctxID, data=text))
            
    router3.set_on_service_capacity_changed_handler(on_service_discovery)
    router3.add_channel(ch2b)
    router2.add_channel(ch1b)

    def signal_handler(signal, frame):
        sel.close()
    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()

if __name__ == "__main__":
    main()
```