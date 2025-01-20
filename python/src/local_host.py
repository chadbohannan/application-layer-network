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
