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
