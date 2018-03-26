import java.util.ArrayList;

/// <summary>
/// Packet class declaration.
/// </summary>
public class Packet {
    static private byte IntXOR(int n) {
        byte cnt = 0x0;
        while (n != 0) { /* This loop will only execute the number times equal to the number of ones. */
            cnt ^= 0x1;
            n &= (n - 0x1);
        }
        return cnt;
    }

    /**
     * - This encodes the CF using a modified Hamming (15,11).
     * - Returns the encoded 11 bit CF as a 15 bit codeword.
     * - If any of the bits in 0xF800 (the top five bits) are on,
     *   then this will return an error of -1, only 0x07FF are
     *   aloud to be on.
     **/
    static public short Encode(short value) {
        /* check the validity of input */
        //if ((value & 0xF800) != 0x0000) 
        //    throw new System.InvalidOperationException("Invalid input");
        /* perform G matrix */
        int temp = (value & 0x07FF) | 
            (IntXOR(value & 0x071D) << 12) |
            (IntXOR(value & 0x04DB) << 13) |
            (IntXOR(value & 0x01B7) << 14) |
            (IntXOR(value & 0x026F) << 15);
        return (short) (0x0000FFFF & temp);
    }

    /**
     * - This decodes the CF using a modified Hamming (15,11).
     * - It will fix one error, if only one error occures,
     *   not very good for burst errors.
     * - This is a SEC (single error correction) which means it has
     *   no BED (bit error detection) to save on size.
     * - The returned value will be, if fixed properly, the same
     *   11 bits that were sent into the encoder.
     * - Bits 0xF800 will be zero.
     **/
    static public short Decode(short value) {
        int err = IntXOR(value & 0x826F) /* perform H matrix */
                | (IntXOR(value & 0x41B7) << 1) | (IntXOR(value & 0x24DB) << 2) | (IntXOR(value & 0x171D) << 3);
        switch (err) /* decode error feild */
        {
        case 0x0F: return (short) (value ^ 0x0001);
        case 0x07: return (short) (value ^ 0x0002);
        case 0x0B: return (short) (value ^ 0x0004);
        case 0x0D: return (short) (value ^ 0x0008);
        case 0x0E: return (short) (value ^ 0x0010);
        case 0x03: return (short) (value ^ 0x0020);
        case 0x05: return (short) (value ^ 0x0040);
        case 0x06: return (short) (value ^ 0x0080);
        case 0x0A: return (short) (value ^ 0x0100);
        case 0x09: return (short) (value ^ 0x0200);
        case 0x0C: return (short) (value ^ 0x0400);
        default: return value;
        }
    }

    public class ControlFlag {
        public static final byte unused4=0x0400;
        public static final byte unused3=0x0200;
        public static final byte unused2=0x0100;
        public static final byte unused1=0x0080;
        public static final byte linkState=0x0040;
        public static final byte scrAddr=0x0020;
        public static final byte destAddr=0x0010;
        public static final byte seqNum=0x0008;
        public static final byte ackBlock=0x0004;
        public static final byte data=0x0002;
        public static final byte crc=0x0001;
    }

    public class LinkState {
        public static final byte CONNECT = 0x01;
        public static final byte CONNECTED = 0x03;
        public static final byte PING = 0x05;
        public static final byte PONG = 0x07;
        public static final byte ACKRESEND = 0x09;
        public static final byte NOACK = 0x0B;
        public static final byte CLOSE = 0x0D;
    }

    public static final byte FrameLeader = 0x3C;
    public static final byte FrameLeaderLength = 4;
    public static final byte FrameEscape = 0xC3;

    public byte LinkState;
    public short SourceAddress;
    public short DestinationAddress;
    public short SequenceNum;
    public int AcknowledgeBlock;
    public byte[] Data;
    
    public Packet() { }

    public Packet(Packet p) {
        LinkState = p.LinkState;
        SourceAddress = p.SourceAddress;
        DestinationAddress = p.DestinationAddress;
        SequenceNum = p.SequenceNum;
        AcknowledgeBlock = p.AcknowledgeBlock;
        Data = p.Data;
    }

    public Packet(byte[] pData) {
        init(pData);
    }

    public static Integer readUINT16(byte[] pData, int offset) {
        Integer value = new Integer(pData[offset] << 8);
        value |= pData[offset + 1];
        return value;
    }

    public static byte[] writeUINT16(Integer value) {
        byte[] bytes = new byte[2];
        bytes[0] = (byte) (value >> 8);
        bytes[1] = (byte) (value & 0x00FF);
        return bytes;
    }

    public static long readUINT32(byte[] pData, int offset) {
        long value = new Integer(pData[offset] << 24);
        value |= (pData[offset + 1] << 16);
        value |= (pData[offset + 2] << 8);
        value |= pData[offset + 3];
        return value;
    }

    public static byte[] writeUINT32(long value) {
        byte[] bytes = new byte[4];
        bytes[0] = (byte) (value >> 24);
        bytes[1] = (byte) (value >> 16 & 0x00FF);
        bytes[2] = (byte) (value >> 8 & 0x00FF);
        bytes[3] = (byte) (value & 0x00FF);
        return bytes;
    }

    public static int fieldOffset(short controlFlags, ControlFlag field) {
        int offset = 2; // ControlFlag bytes
        if (field == ControlFlag.linkState)
            return offset;
        offset += 1;
        if (field == ControlFlag.scrAddr)
            return offset;
        offset += 2;
        if (field == ControlFlag.destAddr)
            return offset;
        offset += 2;
        if (field == ControlFlag.seqNum)
            return offset;
        offset += 2;
        if (field == ControlFlag.ackBlock)
            return offset;
        return offset + 4;
    }

    public static int headerLength(short controlFlags) {
        int len = 2; // control flag bytes
        if ((controlFlags & ControlFlag.linkState) != 0x00) len += 1;
        if ((controlFlags & ControlFlag.scrAddr) != 0x00) len += 2;
        if ((controlFlags & ControlFlag.destAddr) != 0x00) len += 2;
        if ((controlFlags & ControlFlag.seqNum) != 0x00) len += 2;
        if ((controlFlags & ControlFlag.ackBlock) != 0x00) len += 4;
        if ((controlFlags & ControlFlag.data) != 0x00) len += 2;
        return len;
    }

    public void init(byte[] pData) {
        short controlFlags = readUINT16(pData, 0);
        int offset = 2;
        if ((controlFlags & ControlFlag.linkState) != 0) {
            LinkState = pData[offset];
            offset += 1;
        }
        if ((controlFlags & ControlFlag.scrAddr) != 0) {
            SourceAddress = readUINT16(pData, offset);
            offset += 2;
        }
        if ((controlFlags & ControlFlag.destAddr) != 0) {
            DestinationAddress = readUINT16(pData, offset);
            offset += 2;
        }
        if ((controlFlags & ControlFlag.seqNum) != 0) {
            SequenceNum = readUINT16(pData, offset);
            offset += 2;
        }
        if ((controlFlags & ControlFlag.ackBlock) != 0) {
            AcknowledgeBlock = readUINT32(pData, offset);
            offset += 4;
        }
        if ((controlFlags & ControlFlag.data) != 0) {
            int dataLength = readUINT16(pData, offset);
            offset += 2;
            Data = new byte[dataLength];
            for (int i = 0; i < dataLength; i++)
                Data[i] = pData[offset++];
        }
        // evaluate CRC
        if ((controlFlags & ControlFlag.crc) != 0) {
            int crcPacket = readUINT32(pData, offset);
            int crcCalc = CRC32(pData, 0, offset);
            if (crcPacket != crcCalc) {
                String msg = String.Format("CRC error, {0:X8} != {1:X8}", crcPacket, crcCalc);
                Console.Out.WriteLine(msg);
            } else {
                Console.Out.WriteLine("CRC OK");
            }
        }
    }

    public void clear() {
        LinkState = 0;
        SourceAddress = 0;
        DestinationAddress = 0;
        SequenceNum = 0;
        AcknowledgeBlock = 0;
        Data = new byte[] {};
    }

    /// <summary>
    /// returns the hamming encoded Control Field 
    /// </summary>
    public short ControlField() {
        short controlField = 0;
        if (LinkState != 0) controlField |= ControlFlag.linkState;
        if (SourceAddress != 0) controlField |= ControlFlag.scrAddr;
        if (DestinationAddress != 0) controlField |= ControlFlag.destAddr;
        if (SequenceNum != 0) controlField |= ControlFlag.seqNum;
        if (AcknowledgeBlock != 0) controlField |= ControlFlag.ackBlock;
        if (Data.Length != 0) controlField |= ControlFlag.data;
        controlField |= ControlFlag.crc; // TODO enable control of CRC
        controlField = Encode(controlField);
        return controlField;
    }

    /// <summary>
    /// Calculate ELP's prefered 32 bit Cyclic Redundancy Check of a byte array
    /// </summary>
    /// <param name="pdata">The byte array.</param>
    /// <returns>The CRC'ed value of input.</returns>
    static public int CRC32(byte[] pdata, int start, int stop) {
        int crc = 0xFFFFFFFF;
        for (int i = start; i < stop; i++) {
            byte c = pdata[i];
            for (int j = 0x01; j <= 0x80; j <<= 1) {
                int bit = crc & 0x80000000;
                crc <<= 1;
                if ((c & j) > 0)
                    bit ^= 0x80000000;
                if (bit > 0)
                    crc ^= 0x4C11DB7;
            }
        }
        /* Reverse */
        crc = (((crc & 0x55555555) << 1) | ((crc & 0xAAAAAAAA) >> 1));
        crc = (((crc & 0x33333333) << 2) | ((crc & 0xCCCCCCCC) >> 2));
        crc = (((crc & 0x0F0F0F0F) << 4) | ((crc & 0xF0F0F0F0) >> 4));
        crc = (((crc & 0x00FF00FF) << 8) | ((crc & 0xFF00FF00) >> 8));
        crc = (((crc & 0x0000FFFF) << 16) | ((crc & 0xFFFF0000) >> 16));
        return (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /// <summary>
    /// Creates a byte array containing the serialized packet framed for transmission.
    /// </summary>
    /// <returns>The byte stuffed packet buffer ready for transmission.</returns>
    private byte[] toFrameBuffer()
    {
        ArrayList byteBuffer = new ArrayList();
        for (int i = 0; i < FrameLeaderLength; i++)
            byteBuffer.Add(FrameLeader);

        int count = 0;
        ArrayList<byte> buf = toPacketBuffer();
        for (int i = 0; i < buf.length; i++) {
            byteBuffer.Add(buf[i]);
            if (b == FrameLeader) {
                ++count;
                if (count == 3) {
                    byteBuffer.Add((byte)FrameEscape);
                    count = 0;
                }
            }
            else count = 0;
        }
        byte[] returnBuf = new byte[byteBuffer.Count];
        for (int i = 0; i < byteBuffer.Count; i++)
            returnBuf[i] = (byte)(byteBuffer.get(i));
        return returnBuf;
    }

    /// <summary>
    /// returns a byte array of the Packet
    /// </summary>
    public LinkedList toPacketBuffer() {
        LinkedList list = new LinkedList();

        int controlField = ControlField;
        byte[] cf = writeUINT16(controlField);
        for (int i = 0; i < cf.length; i++)
            list.AddLast(cf[i]);

        if ((controlField & ControlFlag.linkState) != 0) {
            list.AddLast(LinkState);
        }
        if ((controlField & ControlFlag.scrAddr) != 0) {
            byte[] buf = writeUINT16(SourceAddress);
            for (int i = 0; i < buf.length; i++)
                list.AddLast(buf[i]);
        }
        if((controlField & ControlFlag.destAddr)!=0) {
            byte[] buf = writeUINT16(DestinationAddress);
            for (int i = 0; i < buf.length; i++)
                list.AddLast(buf[i]);
        }
        if((controlField & ControlFlag.seqNum)!=0) {
            byte[] buf = writeUINT16(SequenceNum);
            for (int i = 0; i < buf.length; i++)
                list.AddLast(buf[i]);
        }
        if((controlField & ControlFlag.ackBlock)!=0) {
            byte[] buf = writeUINT32(AcknowledgeBlock);
            for (int i = 0; i < buf.length; i++)
                list.AddLast(buf[i]);
        }
        if((controlField &  ControlFlag.data)!=0) {
            byte[] buf = writeUINT16(Data.Length);
            for (int i = 0; i < buf.length; i++)
                list.AddLast(buf[i]);

            // then the data
            for (int i = 0; i < Data.length; i++)
                list.AddLast(Data[i]);
        }
        if((controlField & ControlFlag.crc)!=0) {
            int crcValue = CRC32(list);
            byte[] buf = writeUINT32(crcValue);
            for (int i = 0; i < buf.length; i++)
                list.AddLast(buf[i]);
        }

        return list;
    }

    /// <summary>
    /// Summary description for Packet.
    /// </summary>
    public String toString() {
        short controlField = ControlField;
        byte[] cf = BitConverter.GetBytes(controlField);

        StringBuilder sb = new StringBuilder();
        sb.AppendFormat("{0:X2}", cf[1]);
        sb.AppendFormat("{0:X2}", cf[0]);

        if ((controlField & ControlFlag.linkState) != 0)
            sb.AppendFormat("{0:X2}", LinkState);
        if ((controlField & ControlFlag.scrAddr) != 0)
            sb.AppendFormat("{0:X4}", SourceAddress);
        if ((controlField & ControlFlag.destAddr) != 0)
            sb.AppendFormat("{0:X4}", DestinationAddress);
        if ((controlField & ControlFlag.seqNum) != 0)
            sb.AppendFormat("{0:X4}", SequenceNum);
        if ((controlField & ControlFlag.ackBlock) != 0)
            sb.AppendFormat("{0:X8}", AcknowledgeBlock);
        if ((controlField & ControlFlag.data) != 0)
            sb.AppendFormat("{0:X4}", Data.Length);

        return sb.ToString();
    }

}
