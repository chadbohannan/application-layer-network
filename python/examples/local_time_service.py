import selectors, socket, time
from alnmeshpy import TcpChannel, Router, Packet
from datetime import datetime

pong_count = 0
def main():
    sel = selectors.DefaultSelector()
    router = Router(sel, "python-time-source-1")
    router.start()
    print('a')
    # assume a localhost connection on port 8081; see ip_ping_host.py
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8081))
    print('b')
    # join the network
    ch = TcpChannel(sock)
    router.add_channel(ch)
    print('c')
    while True:
        print('d')
        time.sleep(5.0)
        now = datetime.now().isoformat()
        print("sending " + now + " to log services")
        msg = router.send(Packet(service="log", data=now))
        if msg: print("on send:", msg)

if __name__ == "__main__":
    main()
