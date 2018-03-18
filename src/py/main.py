from packet import Packet, readINT32U, writeINT32U
from parser import Parser

# import pdb; pdb.set_trace()

def packet_callback(packet):
    print 'packet parsed with %d data bytes: %s' % (packet.dataSize, "".join(map(chr, packet.data)))
    fname = "packetstream.output"
    with open(fname, "ab") as f:
        f.write("".join(map(chr, packet.toFramedBuffer())))

def main():
    # TODO parse packets from the input file
    fname = "packetstream.input"
    parser = Parser(packet_callback)
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(15), ''):
            parser.readBytes(bytearray(chunk))


if __name__ == "__main__":
    main()
