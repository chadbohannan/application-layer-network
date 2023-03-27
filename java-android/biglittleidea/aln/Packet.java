package biglittleidea.aln;

import android.util.Log;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.zip.CRC32;
import java.util.zip.Checksum;

// Packet class declaration.
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
    static public short HammingEncode(short value) {
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
    static public short HammingDecode(short value) {
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
        public static final int netState  = 0x0400;
        public static final int service   = 0x0200;
        public static final int scrAddr   = 0x0100;
        public static final int destAddr  = 0x0080;
        public static final int nextAddr  = 0x0040;
        public static final int seqNum    = 0x0020;
        public static final int ackBlock  = 0x0010;
        public static final int contextID = 0x0008;
        public static final int dataType  = 0x0004;
        public static final int data      = 0x0002;
        public static final int crc      = 0x0001;
    }

    public static class NetState {
        public static final byte ROUTE   = 0x01; // packet contains route entry
        public static final byte SERVICE = 0x02; // packet contains service entry
        public static final byte QUERY   = 0x03; // packet is a request for content
    }
    
    public byte Net;
    public String Service;
    public String SourceAddress;
    public String DestAddress;
    public String NextAddress;
    public short SequenceNum;
    public int AcknowledgeBlock;
    public short ContextID;
    public byte DataType;
    public byte[] Data;
    public int CRC;

    public Packet() { }

    public Packet(Packet p) {
        Net = p.Net;
        Service = p.Service;
        SourceAddress = p.SourceAddress;
        DestAddress = p.DestAddress;
        NextAddress = p.NextAddress;
        SequenceNum = p.SequenceNum;
        AcknowledgeBlock = p.AcknowledgeBlock;
        ContextID = p.ContextID;
        DataType = p.DataType;
        Data = p.Data; // TODO fix
    }

    public Packet(byte[] pData) {
        init(pData);
    }

    public Packet copy() {
        return new Packet(this);
    }

    public static short readUINT16(byte[] pData, int offset) {
        short value = (short)((pData[offset] << 8) & 0xFF00);
        value |= (pData[offset + 1] & (short)0x00FF);
        return value;
    }

    public static byte[] writeUINT16(int value) {
        byte[] bytes = new byte[2];
        bytes[0] = (byte) (value >> 8);
        bytes[1] = (byte) (value & 0x00FF);
        return bytes;
    }
    
    public static int readUINT32(byte[] pData, int offset) {
        int value = (int)((pData[offset] << 24) & 0xFF000000);
        value |= (int)((pData[offset + 1] << 16) & 0x00FF0000);
        value |= (int)((pData[offset + 2] << 8) & 0x0000FF00);
        value |= (int)((pData[offset + 3]) & 0x000000FF);
        return value;
    }

    public static byte[] writeUINT32(int value) {
        byte[] bytes = new byte[4];
        bytes[0] = (byte) (value >> 24);
        bytes[1] = (byte) (value >> 16 & 0x00FF);
        bytes[2] = (byte) (value >> 8 & 0x00FF);
        bytes[3] = (byte) (value & 0x00FF);
        return bytes;
    }

    public void init(byte[] pData) {
        short controlFlags = readUINT16(pData, 0);
        int offset = 2;
        if ((controlFlags & ControlFlag.netState) != 0) {
            Net = pData[offset];
            offset += 1;
        }
        if ((controlFlags & ControlFlag.service) != 0) {
            int srvSize = pData[offset];
            offset += 1;
            byte[] srvBuf = Arrays.copyOfRange(pData, offset, offset+srvSize);
            offset += srvSize;
            Service = new String(srvBuf);
        }
        if ((controlFlags & ControlFlag.scrAddr) != 0) {
            int addrSize = pData[offset];
            offset += 1;
            byte[] addrBuf = Arrays.copyOfRange(pData, offset, offset+addrSize);
            offset += addrSize;
            SourceAddress = new String(addrBuf);
        }
        if ((controlFlags & ControlFlag.destAddr) != 0) {
            int addrSize = pData[offset];
            offset += 1;
            byte[] addrBuf = Arrays.copyOfRange(pData, offset, offset+addrSize);
            offset += addrSize;
            DestAddress = new String(addrBuf);
        }
        if ((controlFlags & ControlFlag.nextAddr) != 0) {
            int addrSize = pData[offset];
            offset += 1;
            byte[] addrBuf = Arrays.copyOfRange(pData, offset, offset+addrSize);
            offset += addrSize;
            NextAddress = new String(addrBuf);
        }
        if ((controlFlags & ControlFlag.seqNum) != 0) {
            SequenceNum = readUINT16(pData, offset);
            offset += 2;
        }
        if ((controlFlags & ControlFlag.ackBlock) != 0) {
            AcknowledgeBlock = readUINT32(pData, offset);
            offset += 4;
        }
        if ((controlFlags & ControlFlag.contextID) != 0) {
            ContextID = readUINT16(pData, offset);
            offset += 2;
        }
        if ((controlFlags & ControlFlag.dataType) != 0) {
            DataType = pData[offset];
            offset += 1;
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
                // TODO error handling
                Log.d("ALNN", String.format("CRC error, %x != %x\n", crcPacket, crcCalc));
                CRC = -1;
            } else {
                CRC = crcPacket;
            }
        }
    }

    // returns the hamming encoded Control Field
    public short ControlField() {
        short controlField = 0;
        if (Net != 0) controlField |= ControlFlag.netState;
        if (Service != null) controlField |= ControlFlag.service;
        if (SourceAddress != null) controlField |= ControlFlag.scrAddr;
        if (DestAddress != null) controlField |= ControlFlag.destAddr;
        if (NextAddress != null) controlField |= ControlFlag.nextAddr;
        if (SequenceNum != 0) controlField |= ControlFlag.seqNum;
        if (AcknowledgeBlock != 0) controlField |= ControlFlag.ackBlock;
        if (ContextID != 0) controlField |= ControlFlag.contextID;
        if (DataType != 0) controlField |= ControlFlag.dataType;
        if (Data != null && Data.length != 0) controlField |= ControlFlag.data;
        if (CRC != 0) controlField |= ControlFlag.crc;
        controlField = HammingEncode(controlField);
        return controlField;
    }

    // Calculate 32 bit Cyclic Redundancy Check of a byte array
    static public int CRC32(byte[] pdata, int start, int stop) {
        Checksum checksum = new CRC32();
        checksum.update(pdata, start, stop);
        return (int)checksum.getValue();
    }

    // Creates a byte array containing the serialized packet framed for transmission
    public byte[] toFrameBuffer() {
        return Frame.toAX25Buffer(this.toPacketBuffer());
    }

    // returns a byte array of the Packet
    public byte[] toPacketBuffer() {
        ByteBuffer buffer = ByteBuffer.allocate(Frame.BufferSize);
        int controlField = ControlField();
        buffer.put(writeUINT16(controlField));
        if ((controlField & ControlFlag.netState) != 0) {
            buffer.put(Net);
        }
        if ((controlField & ControlFlag.service) != 0) {
            byte[] service = Service.getBytes();
            buffer.put((byte)service.length);
            buffer.put(service);
        }
        if ((controlField & ControlFlag.scrAddr) != 0) {
            byte[] srcAddr = SourceAddress.getBytes();
            buffer.put((byte)srcAddr.length);
            buffer.put(srcAddr);
        }
        if((controlField & ControlFlag.destAddr)!=0) {
            byte[] destAddr = DestAddress.getBytes();
            buffer.put((byte)destAddr.length);
            buffer.put(destAddr);
        }
        if((controlField & ControlFlag.nextAddr)!=0) {
            byte[] nextAddr = NextAddress.getBytes();
            buffer.put((byte)nextAddr.length);
            buffer.put(nextAddr);
        }
        if((controlField & ControlFlag.seqNum)!=0) {
            buffer.put(writeUINT16(SequenceNum));
        }
        if((controlField & ControlFlag.ackBlock)!=0) {
            buffer.put(writeUINT32(AcknowledgeBlock));
        }
        if((controlField & ControlFlag.contextID)!=0) {
            buffer.put(writeUINT16(ContextID));
        }
        if((controlField & ControlFlag.dataType)!=0) {
            buffer.put(DataType);
        }
        if((controlField &  ControlFlag.data)!=0) {
            buffer.put(writeUINT16(Data.length));
            buffer.put(Data);
        }
        if((controlField &  ControlFlag.crc)!=0) {
            buffer.put(writeUINT32(0)); // TODO: not zero
            buffer.put(Data);
        }

        byte[] data = new byte[buffer.position()];
        buffer.rewind();
        buffer.get(data);
        return data;
    }

    // Summary description for Packet.
    public String toString() {
        short controlField = ControlField();
        byte[] cf = writeUINT16(controlField);
        String s = String.format("cf:0x%02x%02x,", cf[0], cf[1]);
        if ((controlField & ControlFlag.netState) != 0)
            s += String.format("net:%d,", Net);
        if ((controlField & ControlFlag.service) != 0)
            s += String.format("srv:%s,", Service);
        if ((controlField & ControlFlag.scrAddr) != 0)
            s += String.format("src:%s,", SourceAddress);
        if ((controlField & ControlFlag.destAddr) != 0)
            s += String.format("dst:%s,", DestAddress);
        if ((controlField & ControlFlag.nextAddr) != 0)
            s += String.format("nxt:%s,", NextAddress);
        if ((controlField & ControlFlag.seqNum) != 0)
            s += String.format("seq:%d,", SequenceNum);
        if ((controlField & ControlFlag.ackBlock) != 0)
            s += String.format("ack:0x%x,", AcknowledgeBlock);
        if ((controlField & ControlFlag.contextID) != 0)
            s += String.format("ctx:%d,", ContextID);
        if ((controlField & ControlFlag.dataType) != 0)
            s += String.format("typ:%d,", DataType);
        if ((controlField & ControlFlag.data) != 0)
            s += String.format("len:%d", Data.length);
        if ((controlField & ControlFlag.crc) != 0)
            s += String.format("crc:0x%x", CRC);
        return s;
    }

}
