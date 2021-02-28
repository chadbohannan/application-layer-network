import sys, time
sys.path.insert(0, "../../src/py")
from aln.packet import Packet, readINT32U, writeINT32U
from aln.parser import Parser

# import pdb; pdb.set_trace()

def packet_callback(packet):
    # import pdb; pdb.set_trace()
    print("".join(packet.data))

def main():
    packet = Packet(None)
    packet.serviceID = 1
    packet.data = "ping"

    buff = packet.toFramedBuffer()
    print("ping...")
    
    # TODO use routers to connect a client to a ping service
    parser = Parser(packet_callback)
    parser.readBytes(packet.toFramedBuffer())
    
    time.sleep(.01)

if __name__ == "__main__":
    main()
