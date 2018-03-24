using System;
using System.Collections.Generic;
using System.Text;

namespace ELP
{
	/// <summary>
	/// Packet class declaration.
	/// </summary>
	public class Packet
	{
        class CFHammer
        {
            /**
             * - This class is used to encode and decode the Control Flags (CF) for MAIA's Link Protocol.
             * - (c) 2006 Grant Nelson - MSU/MSGC/SSEL/C&DH
             * - Based off of the following Hamming (15,11) matrix...
             * 
             * G[16,11] = [[1000,0000,0000,1111],  0x800F
             *             [0100,0000,0000,0111],  0x4007
             *             [0010,0000,0000,1011],  0x200B
             *             [0001,0000,0000,1101],  0x100D
             *             [0000,1000,0000,1110],  0x080E
             *             [0000,0100,0000,0011],  0x0403
             *             [0000,0010,0000,0101],  0x0205
             *             [0000,0001,0000,0110],  0x0106
             *             [0000,0000,1000,1010],  0x008A
             *             [0000,0000,0100,1001],  0x0049
             *             [0000,0000,0010,1100]]; 0x002C
             * 
             * H[16,4]  = [[1011,1000,1110,1000],  0x171D
             *             [1101,1011,0010,0100],  0x24DB
             *             [1110,1101,1000,0010],  0x41B7
             *             [1111,0110,0100,0001]]; 0x826F
             **/

            /**
             * - This is a modified "Sparse Ones Bit Count", because
             *   there are 7 or less ones out of 16 bits it is best to look at the ones.
             * - Instead of counting the ones it just determines if there was
             *   an odd or even count of bits by XORing the int.
             * - Returns 0x0 or 0x1.
             **/
            static private byte IntXOR(int n)
            {
                byte cnt = 0x0;
                while (n != 0)
                {   /* This loop will only execute the number times equal to the number of ones. */
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
            static public ushort Encode(ushort value)
            {
                /* check the validity of input */
                //if ((value & 0xF800) != 0x0000) 
                //    throw new System.InvalidOperationException("Invalid input");
                /* perform G matrix */
                value = (ushort)(value & (ushort)0x07FF);
                int temp = value | (IntXOR(value & 0x071D) << 12)
                                 | (IntXOR(value & 0x04DB) << 13)
                                 | (IntXOR(value & 0x01B7) << 14)
                                 | (IntXOR(value & 0x026F) << 15);
                return (ushort) (0x0000FFFF & temp);
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
            static public ushort Decode(ushort value)
            {
                int err = IntXOR(value & 0x826F) /* perform H matrix */
                       | (IntXOR(value & 0x41B7) << 1)
                       | (IntXOR(value & 0x24DB) << 2)
                       | (IntXOR(value & 0x171D) << 3);
                switch (err) /* decode error feild */
                {
                    case 0x0F: return (ushort)(value ^ 0x0001);
                    case 0x07: return (ushort)(value ^ 0x0002);
                    case 0x0B: return (ushort)(value ^ 0x0004);
                    case 0x0D: return (ushort)(value ^ 0x0008);
                    case 0x0E: return (ushort)(value ^ 0x0010);
                    case 0x03: return (ushort)(value ^ 0x0020);
                    case 0x05: return (ushort)(value ^ 0x0040);
                    case 0x06: return (ushort)(value ^ 0x0080);
                    case 0x0A: return (ushort)(value ^ 0x0100);
                    case 0x09: return (ushort)(value ^ 0x0200);
                    case 0x0C: return (ushort)(value ^ 0x0400);
                    default: return value;
                }
            }
        }

        public enum ControlFlag : ushort
        {
            unused4 = 0x0400,
            unused3 = 0x0200,
            unused2 = 0x0100,
            unused1 = 0x0080,
            linkState = 0x0040,
            scrAddr = 0x0020,
            destAddr = 0x0010,
            seqNum = 0x0008,
            ackBlock = 0x0004,
            data = 0x0002,
            crc = 0x0001
        }

        public enum LinkStateEnum : byte
        {
            CONNECT = 0x01,
            CONNECTED = 0x03,
            PING = 0x05,
            PONG = 0x07,
            ACKRESEND = 0x09,
            NOACK = 0x0B,
            CLOSE = 0x0D
        }

        #region CONSTANTS
        public const byte FrameLeader = 0x3C;
        public const byte FrameLeaderLength = 4;
        public const byte FrameEscape = 0xC3;
        #endregion

        #region VARIABLES
        public byte LinkState;
        public ushort SourceAddress;
        public ushort DestinationAddress;
        public ushort SequenceNum;
        public uint AcknowledgeBlock;
        public byte[] Data;
		#endregion CLASS VARIABLES

        #region CONSTRUCTORS...

        /// <summary>
        /// Default Constructor
        /// </summary>
		public Packet() { }

        /// <summary>
        /// Copy Constructor
        /// </summary>
        public Packet(Packet p)
        {
            LinkState = p.LinkState;
            SourceAddress = p.SourceAddress;
            DestinationAddress = p.DestinationAddress;
            SequenceNum = p.SequenceNum;
            AcknowledgeBlock = p.AcknowledgeBlock;
            Data = p.Data;
        }

        /// <summary>
        /// Packet Buffer Constructor
        /// </summary>
        public Packet(byte[] pData)
        {
            init(pData);
        }
        #endregion CONSTRUCTORS

        #region INITIALIZATION

        public static ushort readUINT16(byte[] pData, int offset)
        {
            ushort value = (ushort) (pData[offset] << 8);
            value |= pData[offset + 1];
            return value;
        }

        public static byte[] writeUINT16(ushort value)
        {
            byte[] bytes = new byte[2];
            bytes[0] = (byte)(value >> 8);
            bytes[1] = (byte)(value & 0x00FF);
            return bytes;
        }

        public static uint readUINT32(byte[] pData, int offset)
        {
            uint value = (uint)(pData[offset] << 24);
            value |= (uint)(pData[offset + 1] << 16);
            value |= (uint)(pData[offset + 2] << 8);
            value |= pData[offset + 3];
            return value;
        }

        public static byte[] writeUINT32(uint value)
        {
            byte[] bytes = new byte[4];
            bytes[0] = (byte)(value >> 24);
            bytes[1] = (byte)(value >> 16 & 0x00FF);
            bytes[2] = (byte)(value >> 8 & 0x00FF);
            bytes[3] = (byte)(value & 0x00FF);
            return bytes;
        }

        public static int fieldOffset(ushort controlFlags, ControlFlag field)
        {
            int offset = 2; // ControlFlag bytes
            if (field == ControlFlag.linkState) return offset;
            offset += 1;
            if (field == ControlFlag.scrAddr) return offset;
            offset += 2;
            if (field == ControlFlag.destAddr) return offset;
            offset += 2;
            if (field == ControlFlag.seqNum) return offset;
            offset += 2;
            if (field == ControlFlag.ackBlock) return offset;
            return offset + 4;
        }

        public static int headerLength(ushort controlFlags)
        {
            int len = 2; // 
            if ((controlFlags & (ushort)ControlFlag.linkState) != 0x00) len += 1;
            if ((controlFlags & (ushort)ControlFlag.scrAddr) != 0x00) len += 2;
            if ((controlFlags & (ushort)ControlFlag.destAddr) != 0x00) len += 2;
            if ((controlFlags & (ushort)ControlFlag.seqNum) != 0x00) len += 2;
            if ((controlFlags & (ushort)ControlFlag.ackBlock) != 0x00) len += 4;
            if ((controlFlags & (ushort)ControlFlag.data) != 0x00) len += 2;
            return len;
        }

        /// <summary>
        /// Parse packet buffer into local variables
        /// </summary>
        public void init(byte[] pData)
        {
            ushort controlFlags = readUINT16(pData, 0);
            int offset = 2;
            if ((controlFlags & (ushort)ControlFlag.linkState) != 0)
            {
                LinkState = pData[offset];
                offset += 1;
            }
            if ((controlFlags & (ushort)ControlFlag.scrAddr) != 0)
            {
                SourceAddress = readUINT16(pData, offset);
                offset += 2;
            }
            if ((controlFlags & (ushort)ControlFlag.destAddr) != 0)
            {
                DestinationAddress = readUINT16(pData, offset);
                offset += 2;
            }
            if ((controlFlags & (ushort)ControlFlag.seqNum) != 0)
            {
                SequenceNum = readUINT16(pData, offset);
                offset += 2;
            }
            if ((controlFlags & (ushort)ControlFlag.ackBlock) != 0)
            {
                AcknowledgeBlock = readUINT32(pData, offset);
                offset += 4;
            }
            if ((controlFlags & (ushort)ControlFlag.data) != 0)
            {
                ushort dataLength = readUINT16(pData, offset);
                offset += 2;
                Data = new byte[dataLength];
                for(int i = 0; i < dataLength; i++)
                    Data[i] = pData[offset++];
            }
            // TODO evaluate CRC
            if ((controlFlags & (ushort)ControlFlag.crc) != 0)
            {
                uint crcPacket = readUINT32(pData, offset);
                uint crcCalc = CRC32(pData, 0, (ushort)offset);
                if (crcPacket != crcCalc)
                {
                    String msg = String.Format("CRC error, {0:X8} != {1:X8}", crcPacket, crcCalc);
                    Console.Out.WriteLine(msg);
                }
                else
                {
                    Console.Out.WriteLine("CRC OK");
                }
            }
        }

        public void clear()
        {
            LinkState = 0;
            SourceAddress = 0;
            DestinationAddress = 0;
            SequenceNum = 0;
            AcknowledgeBlock = 0;
            Data = new byte[]{ };
        }
        #endregion

        #region ACCESORS

        /// <summary>
        /// returns the hamming encoded Control Field 
        /// </summary>
        public ushort ControlField
        {
            get {
                ushort controlField = 0;
                if (LinkState != 0) controlField |= (ushort)ControlFlag.linkState;
                if (SourceAddress != 0) controlField |= (ushort)ControlFlag.scrAddr;
                if (DestinationAddress != 0) controlField |= (ushort)ControlFlag.destAddr;
                if (SequenceNum != 0) controlField |= (ushort)ControlFlag.seqNum;
                if (AcknowledgeBlock != 0) controlField |= (ushort)ControlFlag.ackBlock;
                if (Data.Length != 0) controlField |= (ushort)ControlFlag.data;
                controlField |= (ushort)ControlFlag.crc; // TODO enable control of CRC
                controlField = CFHammer.Encode(controlField);
                return controlField;
            }
        }
        
        #endregion ACCESSORS

        #region ERROR DETECTION
        /// <summary>
        /// Calculate ELP's prefered 32 bit Cyclic Redundancy Check of a byte array
        /// </summary>
        /// <param name="pdata">The byte array.</param>
        /// <returns>The CRC'ed value of input.</returns>
        static public uint CRC32(byte[] pdata, ushort start, ushort stop)
        {
            uint crc = 0xFFFFFFFF;
            for (ushort i = start; i < stop; i++)
            {
                byte c = pdata[i];
                for (ushort j = 0x01; j <= 0x80; j <<= 1)
                {
                    uint bit = crc & 0x80000000;
                    crc <<= 1;
                    if ((c & j) > 0) bit ^= 0x80000000;
                    if (bit > 0) crc ^= 0x4C11DB7;
                }
            }
            /* Reverse */
            crc = (((crc & 0x55555555) <<  1) | ((crc & 0xAAAAAAAA) >>  1));
            crc = (((crc & 0x33333333) <<  2) | ((crc & 0xCCCCCCCC) >>  2));
            crc = (((crc & 0x0F0F0F0F) <<  4) | ((crc & 0xF0F0F0F0) >>  4));
            crc = (((crc & 0x00FF00FF) <<  8) | ((crc & 0xFF00FF00) >>  8));
            crc = (((crc & 0x0000FFFF) << 16) | ((crc & 0xFFFF0000) >> 16));
            return (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF;
        }

        static public uint CRC32(LinkedList<byte> buffer)
        {
            uint crc = 0xFFFFFFFF;
            foreach (byte c in buffer)
            {
                for (int j = 0x01; j <= 0x80; j <<= 1)
                {
                    uint bit = crc & 0x80000000;
                    crc <<= 1;
                    if ((c & j) > 0) bit ^= 0x80000000;
                    if (bit > 0) crc ^= 0x4C11DB7;
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

        #endregion


        /// <summary>
        /// Creates a byte array containing the serialized packet framed for transmission.
        /// </summary>
        /// <returns>The byte stuffed packet buffer ready for transmission.</returns>
        private byte[] toFrameBuffer()
        {
            System.Collections.ArrayList byteBuffer = new System.Collections.ArrayList();
            for (int i = 0; i < FrameLeaderLength; i++)
                byteBuffer.Add(FrameLeader);

            int count = 0;
            LinkedList<byte> buf = toPacketBuffer();
            foreach (byte b in buf)
            {
                byteBuffer.Add(b);
                if (b == FrameLeader)
                {
                    ++count;
                    if (count == 3)
                    {
                        byteBuffer.Add((byte)FrameEscape);
                        count = 0;
                    }
                }
                else count = 0;
            }
            byte[] returnBuf = new byte[byteBuffer.Count];
            for (int i = 0; i < byteBuffer.Count; i++)
                returnBuf[i] = (byte)(byteBuffer[i]);
            return returnBuf;
        }

        /// <summary>
        /// returns a byte array of the Packet
        /// </summary>
        public LinkedList<byte> toPacketBuffer()
        {
            LinkedList<byte> list = new LinkedList<byte>();

            ushort controlField = ControlField;
            foreach (byte b in writeUINT16(controlField))
                list.AddLast(b);

            if ((controlField & (ushort)ControlFlag.linkState) != 0)
            {
                list.AddLast(LinkState);
            }
            if ((controlField & (ushort)ControlFlag.scrAddr) != 0)
            {
                foreach (byte b in writeUINT16(SourceAddress))
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlFlag.destAddr) != 0)
            {
                foreach (byte b in writeUINT16(DestinationAddress))
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlFlag.seqNum) != 0)
            {
                foreach (byte b in writeUINT16(SequenceNum))
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlFlag.ackBlock) != 0)
            {
                foreach (byte b in writeUINT32(AcknowledgeBlock))
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlFlag.data) != 0)
            {
                // first the data size
                foreach (byte b in writeUINT16((ushort)Data.Length))
                    list.AddLast(b);

                // then the data
                foreach (byte b in Data)
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlFlag.crc) != 0)
            {
                uint crcValue = CRC32(list);
                foreach (byte b in writeUINT32(crcValue))
                    list.AddLast(b);
            }
            
            return list;
        }

        /// <summary>
        /// Summary description for Packet.
        /// </summary>
        public string toString()
        {
            ushort controlField = ControlField;
            byte[] cf = BitConverter.GetBytes(controlField);

            StringBuilder sb = new StringBuilder();
            sb.AppendFormat("{0:X2}", cf[1]);
            sb.AppendFormat("{0:X2}", cf[0]);

            if ((controlField & (ushort)ControlFlag.linkState) != 0)
                sb.AppendFormat("{0:X2}", LinkState);
            if ((controlField & (ushort)ControlFlag.scrAddr) != 0)
                sb.AppendFormat("{0:X4}", SourceAddress);
            if ((controlField & (ushort)ControlFlag.destAddr) != 0)
                sb.AppendFormat("{0:X4}", DestinationAddress);
            if ((controlField & (ushort)ControlFlag.seqNum) != 0)
                sb.AppendFormat("{0:X4}", SequenceNum);
            if ((controlField & (ushort)ControlFlag.ackBlock) != 0)
                sb.AppendFormat("{0:X8}", AcknowledgeBlock);
            if ((controlField & (ushort)ControlFlag.data) != 0)
                sb.AppendFormat("{0:X4}", Data.Length);
            
            return sb.ToString();
        }


	}
}