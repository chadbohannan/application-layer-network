import selectors, signal, socket, sys
from math import remainder
from socket import AF_INET, SOCK_DGRAM
from threading import Lock
from alnmeshpy import TcpChannel, Router, Packet
from urllib.parse import urlparse

# This example listens for a host advertisment to be broadcast by UDP
# on port 8082 and expects a well-formed url like:
# tcp+aln://192.168.0.2:8081
# The url is parsed and the scheme, host, and port used to make a TCP
# connection with the advertised host and the new connection is added
# to the Router

def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-logger-01") # TODO use UUID
    router.start()

    def on_log(packet):
        print('log: ' + packet.data.decode('utf-8'))
    router.register_service("log", on_log)
    
    alnUrl = ""

    if len(sys.argv) > 1:
        print("connecting to:", sys.argv[1])
        alnUrl = sys.argv[1]
    else:
        print("listening to port 8082 for UDP broadcast")

        # listen for broadcast
        s=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind(('', 8082))
        while True:
            m = s.recvfrom(4096)
            alnUrl = m[0].decode('utf-8')
            url = urlparse(alnUrl)
            malnAddr = url.path
            supportedSchemes = ['tcp+aln', 'tcp+maln', 'tls+aln', 'tls+maln']
            if url.scheme in supportedSchemes:
                break
            else:
                print(url.scheme, "not supported. supported schemes are", supportedSchemes)
            break
        s.close()   
    
    print('parsed url params:', url.scheme, url.hostname, url.port, malnAddr)   

    # connect to an existing node in the network
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((url.hostname, url.port))

    # join the network
    ch = TcpChannel(sock)
    if "maln" in url.scheme: # support multiplexed links
        ch.send(Packet(destAddr=malnAddr))
    router.add_channel(ch)
    
    # listen for ^C
    lock = Lock()
    def signal_handler(signal, frame):
        router.close()
        sel.close()
        lock.release() # release the main thread
    signal.signal(signal.SIGINT, signal_handler)

    # release lock to exit if channel is closed
    ch.on_close(lambda x: lock.release())

    # hang until ^C
    lock.acquire() # take the lock
    lock.acquire() # enqueue a lock request to block the application
    lock.release() # pong response recieved; clear the lock and exit

if __name__ == "__main__":
    main()
