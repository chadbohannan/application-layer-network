package org.biglittleidea.aln;

public class Parser {

    private class State {
        public static final int FindStart = 0;
        public static final int GetCF = 1;
        public static final int GetHeader = 2;
        public static final int GetData = 3;
        public static final int GetCRC = 4;
    }

    private byte delimCount; // The current count of sequental delimiters.
    private int state;
    byte[] buffer; // byte buffer a Packet is parsed from
    private short controlFlags;
    private int headerIndex;
    private int headerLength;
    private int dataIndex;
    private int dataLength;
    private int crcIndex;
    int crcLength = 4;
    int MaxHeaderSize = 19;
    int MaxDataSize = 1 << 16;
    int MaxPacketSize = MaxHeaderSize + MaxDataSize + crcLength;

    IPacketHandler handler;

    public Parser(IPacketHandler handler) {
        this.handler = handler;
        buffer = new byte[MaxPacketSize];
        reset();
    }

    public void reset() {
        state = State.FindStart;
        headerIndex = 0;
        headerLength = 0;
        dataIndex = 0;
        dataLength = 0;
        crcIndex = 0;
    }

    public int readBytes(byte[] data, int length)
        {
            for (int i = 0; i < length; i++)
            {
                byte b = data[i];
                // a simple state machine to strip framming bytes from incoming data
                switch(b)
                {
                    case Packet.FrameLeader:
                        delimCount += 1;
                        if (delimCount >= Packet.FrameLeaderLength) // frame leader detected
                        {
                            reset();
                            continue;
                        }
                        break;
                    case Packet.FrameEscape:
                        if (delimCount == Packet.FrameLeaderLength - 1)
                        {
                            delimCount = 0;
                            continue; // drop frame-escape char independent of the packet parser state
                        }
                        // implicity fallthrough (goto default)
                    default: // byte is not a FrameLeader
                        if (delimCount >= Packet.FrameLeaderLength) // first packet byte detected
                        {
                            state = State.GetCF;
                            delimCount = 0;
                        }
                        break;
                }

                // a more complex state machine to parse a packet from an input stream
                switch (state)
                {
                    case State.FindStart:
                        break; // drop framing bytes
                    case State.GetCF:
                        GetCF(b); // read
                        break;
                    case State.GetHeader:
                        GetHeader(b);
                        break;
                    case State.GetData:
                        GetData(b);
                        break;
                    case State.GetCRC:
                        GetCRC(b);
                        break;
                }
            }
            return data.length;
        }

    private void GetCF(byte b) {
        buffer[headerIndex] = b;
        headerIndex += 1;
        if (headerIndex == 2) {
            controlFlags = Packet.readUINT16(buffer, 0);
            headerLength = Packet.headerLength(controlFlags);
            if (headerLength > 2) {
                state = State.GetHeader;
            } else {
                headerIndex = 0;
                state = State.FindStart;
            }
        }
    }

    private void GetHeader(byte b) {
        buffer[headerIndex] = b;
        headerIndex += 1;
        if (headerIndex == headerLength) {
            if ((controlFlags & Packet.ControlFlag.data) != 0) {
                dataLength = Packet.readUINT16(buffer, headerIndex - 2);
                dataIndex = 0;
                state = State.GetData;
            } else if ((controlFlags & Packet.ControlFlag.crc) != 0) {
                dataLength = 0;
                dataIndex = 0;
                crcIndex = 0;
                state = State.GetCRC;
            } else {
                handler.onPacket(new Packet(buffer));
                reset();
            }
        }
    }

    private void GetData(byte b) {
        int offset = headerIndex + dataIndex;
        buffer[offset] = b;
        dataIndex += 1;
        if (dataIndex == dataLength) {
            if ((controlFlags & Packet.ControlFlag.crc) != 0) {
                crcIndex = 0;
                state = State.GetCRC;
            } else {
                handler.onPacket(new Packet(buffer));
                reset();
            }
        }
    }

    private void GetCRC(byte b) {
        int crcStart = headerIndex + dataIndex;
        buffer[crcStart + crcIndex] = b;
        crcIndex += 1;
        if (crcIndex == crcLength) {
            handler.onPacket(new Packet(buffer));
            reset();
        }
    }

}
