/**********************************************************************************************************************
 *                                         Space Science & Engineering Lab - MSU
 *                                           Maia University Nanosat Program
 *
 *                                                    IMPLEMENTATION
 * Filename      : PacketHeader.cs
 * Programmer(s) : Ehson Mosleh (ehson@ssel.montana.edu), Chad Bohannan
 * Created       : 22 Aug, 2006
 * Description   : The Packet Header for a Packet object in the SSEL Link Communication Protocol. 
 *                  The header of the packet has a delimiter that will denote the beginning of a 
 *                  packet and will also hold vital information about the routing of the packet, 
 *                  Session Management, error detection as well as the size of the body of the packet.
 *                 This class enables the creation and manipulation of packet headers as well as 
 *                  functions to represent a header as a string and a byte array. 
 **********************************************************************************************************************/
using System;
using System.Collections.Generic;
using System.Text;


namespace ELP
{
	/// <summary>
	/// PacketHeader class declaration.
	/// </summary>
	public class PacketHeader
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
        class CFHammer
        {
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

        public enum ControlMask : ushort
        {
            ZERO = 0x0800,
            unused2 = 0x0400,
            unused1 = 0x0200,
            linkState = 0x0100,
            timeStamp = 0x0080,
            asciiCallSign = 0x0040,
            scrAddr = 0x0020,
            destAddr = 0x0010,
            seqNum = 0x0008,
            ackBlock = 0x0004,
            errorType = 0x0002,
            dataSize = 0x0001
        }

        public enum LinkState : byte
        {
            CONNECT = 0x01,
            CONNECTED = 0x03,
            PING = 0x05,
            PONG = 0x07,
            ACKRESEND = 0x09,
            NOACK = 0x0B,
            CLOSE = 0x0D
        }

        public enum ErrorType : byte
        {
            NO_CRC = 0x03,
            CRC16 = 0x1C,
            CRC32 = 0xFF,
            CRC_CCITT = 0xE0
        }

		#region CLASS VARIABLES...

        private uint startDelimeter;
        private UInt16 controlField;
        private uint timeStamp;
		private byte[] asciiCallSign = new byte[8];
		private ushort sourceAddress;
		private ushort destinationAddress;
		private ushort sequenceNum;
		private uint acknowledgeBlock;
		private byte errorDetectType;
		private ushort dataLength;
        private uint crc;
        private byte linkState;

		#endregion CLASS VARIABLES

		#region CONSTANTS...

		#endregion CONSTANTS

        #region CONSTRUCTORS...

        /// <summary>
        /// Default Constructor
        /// </summary>
		public PacketHeader()
		{
            startDelimeter = 0x3c3c3c3c;
            controlField = 0x0000;
        }

        /// <summary>
        /// Copy Constructor
        /// </summary>
        public PacketHeader(PacketHeader p)
        {
            startDelimeter = p.StartDelimeter;
            controlField = p.ControlField;
            linkState = p.linkState;
            timeStamp = p.TimeStamp;
            asciiCallSign = p.AsciiCallSign;
            sourceAddress = p.SourceAddress;
            destinationAddress = p.DestinationAddress;
            sequenceNum = p.SequenceNum;
            acknowledgeBlock = p.AcknowledgeBlock;
            errorDetectType = p.ErrorDetectType;
            dataLength = p.DataLength;
            crc = p.crc;
        }

        #endregion CONSTRUCTORS

        #region ACCESOR METHODS...

        /// <summary>
        /// Get and Set the linkState field
        /// </summary>
        public byte LinkStateProp
        {
            get { return linkState; }
            set 
            { 
                linkState = value;
                controlField |= (ushort)ControlMask.linkState;
                controlField = CFHammer.Encode(controlField);
            }
        }

        public bool HasLinkStateProp()
        {
            if ((this.controlField & (ushort)ControlMask.linkState) != 0)
                return true;
            else
                return false;
        }

        /// <summary>
        /// Get and Set the StartDelimeter
        /// </summary>
        public uint StartDelimeter
        {
            get { return startDelimeter; }
            set { startDelimeter = value; }
        }

        /// <summary>
        /// Get and Set the TimeStamp field
        /// </summary>
        public uint TimeStamp
        {
            get { return timeStamp; }
            set
            {
                timeStamp = value;
                controlField |= (ushort)ControlMask.timeStamp;
                controlField = CFHammer.Encode(controlField);
            }
        }

        public bool HasTimeStamp()
        {
            if ((this.controlField & (ushort)ControlMask.timeStamp) != 0)
                return true;
            else
                return false;
        }

        /// <summary>
        /// returns the Control Field of the Header, hamming encoded
        /// </summary>
        public ushort ControlField
        {
            get { return controlField; }
            set { controlField = value; }
        }

        /// <summary>
        /// gets and sets the ASCIICallSign
        /// </summary>
        public byte[] AsciiCallSign
        {
            get { return asciiCallSign; }
            set
            {
                asciiCallSign = value;
                controlField |= (ushort)ControlMask.asciiCallSign;
                controlField = CFHammer.Encode(controlField);
            }
        }

        public bool HasAsciiCallSign()
        {
            if ((this.controlField & (ushort)ControlMask.asciiCallSign) != 0)
                return true;
            else
                return false;
        }

        /// <summary>
        /// Get and Set the Source address of the packet
        /// </summary>
        public ushort SourceAddress
        {
            get { return sourceAddress; }
            set
            {
                sourceAddress = value;
                controlField |= (ushort)ControlMask.scrAddr;
                controlField = CFHammer.Encode(controlField);
            }
        }

        /// <summary>
        /// Get and Set the Destination Address of the packet
        /// </summary>
        public ushort DestinationAddress
        {
            get { return destinationAddress; }
            set
            {
                destinationAddress = value;
                controlField |= (ushort)ControlMask.destAddr;
                controlField = CFHammer.Encode(controlField);
            }
        }

        /// <summary>
        /// Get and Set the Sequence Number of the Packet
        /// </summary>
        public ushort SequenceNum
        {
            get { return sequenceNum; }
            set
            {
                sequenceNum = value;
                controlField |= (ushort)ControlMask.seqNum;
                controlField = CFHammer.Encode(controlField);
            }
        }

        public bool HasSequenceNum()
        {
            if ((this.controlField & (ushort)ControlMask.seqNum) != 0)
                return true;
            else
                return false;
        }

        /// <summary>
        /// Get and set the Acknowledge Block for the Packet
        /// </summary>
        public uint AcknowledgeBlock
        {
            get { return acknowledgeBlock; }
            set
            {
                acknowledgeBlock = value;
                controlField |= (ushort)ControlMask.ackBlock;
                controlField = CFHammer.Encode(controlField);
            }
        }

        public bool HasAcknowledgeBlock()
        {
            if ((this.controlField & (ushort)ControlMask.ackBlock) != 0)
                return true;
            else
                return false;
        }

        /// <summary>
        /// Get and Set the Error Detect Type for the Packet Header and 
        /// the Packet data section
        /// </summary>
        public byte ErrorDetectType
        {
            get { return errorDetectType; }
            set 
            { 
                errorDetectType = value;
                controlField |= (ushort)ControlMask.errorType;
                controlField = CFHammer.Encode(controlField);
            }
        }

        /// <summary>
        /// Get and Set the number of bytes in the Data field of the packet 
        /// </summary>
        public ushort DataLength
        {
            get { return dataLength; }
            set
            {
                dataLength = value;
                controlField |= (ushort)ControlMask.dataSize;
                controlField = CFHammer.Encode(controlField);
            }
        }

        public bool HasData()
        {
            if ((this.controlField & (ushort)ControlMask.dataSize) != 0)
                return true;
            else
                return false;
        }

        /// <summary>
        /// Gets and Sets the Header CRC for the Packet
        /// </summary>
        public uint Crc
        {
            get
            {
                byte[] temp = this.toByteArray();
                crc = getCRC((ErrorType)ErrorDetectType, temp, (ushort)temp.Length);
                return crc;  
            }
            set { crc = value; }
        }

        public byte[] getCRCBytes()
        {
            byte[] crcBytes;
            if (getCRCLength(ErrorDetectType) == 2)
                crcBytes = BitConverter.GetBytes((ushort)(0x0000FFFF & Crc));
            else if (getCRCLength(ErrorDetectType) == 4)
                crcBytes = BitConverter.GetBytes(Crc);
            else
                crcBytes = new byte[0];
            Array.Reverse(crcBytes);
            return crcBytes;
        }

        #endregion ACCESSOR METHODS

        #region ERROR DETECTION
        /// <summary>
        /// This CRC's a byte array with the given format.
        /// </summary>
        /// <param name="format">The format type of the CRC.</param>
        /// <param name="pdata">The byte array.</param>
        /// <param name="size">The length to CRC.</param>
        /// <returns>The CRC'ed value. (check sizing before writing this value)</returns>
        static public uint getCRC(ErrorType format, byte[] pdata, ushort length)
        {
            ushort size = length;
            if (format == ErrorType.NO_CRC) return 0x0000;
            byte c;
            ushort i, j;
            uint bit, crc;
            if (format == ErrorType.CRC_CCITT)
            {
                crc = 0xFFFF;
                for (i = 0; i < size; i++)
                {
                    c = pdata[i];
                    for (j = 0x80; j > 0x00; j >>= 1)
                    {
                        bit = crc & 0x8000;
                        crc <<= 1;
                        if ((c & j) > 0) bit ^= 0x8000;
                        if (bit > 0) crc ^= 0x1021;
                    }
                }
                return crc & 0xFFFF;
            }
            else if (format == ErrorType.CRC16)
            {
                crc = 0x0000;
                for (i = 0; i < size; i++)
                {
                    c = pdata[i];
                    for (j = 0x01; j <= 0x80; j <<= 1)
                    {
                        bit = crc & 0x8000;
                        crc <<= 1;
                        if ((c & j) > 0) bit ^= 0x8000;
                        if (bit > 0) crc ^= 0x8005;
                    }
                }
                /* Reverse */
                crc = (((crc & 0x5555) << 1) | ((crc & 0xAAAA) >> 1));
                crc = (((crc & 0x3333) << 2) | ((crc & 0xCCCC) >> 2));
                crc = (((crc & 0x0F0F) << 4) | ((crc & 0xF0F0) >> 4));
                crc = (((crc & 0x00FF) << 8) | ((crc & 0xFF00) >> 8));
                return crc & 0xFFFF;
            }
            else // ErrorType.CRC32
            {
                crc = 0xFFFFFFFF;
                for (i = 0; i < size; i++)
                {
                    c = pdata[i];
                    for (j = 0x01; j <= 0x80; j <<= 1)
                    {
                        bit = crc & 0x80000000;
                        crc <<= 1;
                        if ((c & j) > 0) bit ^= 0x80000000;
                        if (bit > 0) crc ^= 0x41C11DB7;
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
        }

        static public ushort getCRCLength(ErrorType errorType) 
        {
            return getCRCLength((ErrorType) errorType);
        }

        static public ushort getCRCLength(byte errorType)
        {
            ushort length = 0;
            if ((errorType == (byte)ErrorType.CRC_CCITT) ||
                (errorType == (byte)ErrorType.CRC16))
                length = 2;
            else if (errorType == (byte)ErrorType.CRC32)
                length = 4;
            return length;
        }

        #endregion

        /// <summary>
        /// returns a byte array of the PacketHeader
        /// </summary>
        public byte[] toByteArray()
        {

            LinkedList<byte> list = new LinkedList<byte>();

            byte[] sd = BitConverter.GetBytes(startDelimeter);
            Array.Reverse(sd);
            foreach (byte b in sd)
                list.AddLast(b);

            byte[] cf = BitConverter.GetBytes(controlField);
            list.AddLast(cf[1]);
            list.AddLast(cf[0]);

            if ((controlField & (ushort)ControlMask.linkState) != 0)
            {
                list.AddLast(linkState);                
            }
            
            if ((controlField & (ushort)ControlMask.timeStamp) != 0)
            {
                byte[] ts = BitConverter.GetBytes(timeStamp);
                Array.Reverse(ts);
                foreach (byte b in ts)
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlMask.asciiCallSign) != 0)
            {
                foreach (byte b in asciiCallSign)
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlMask.scrAddr) != 0)
            {
                byte[] sa = BitConverter.GetBytes(sourceAddress);
                Array.Reverse(sa);
                foreach (byte b in sa)
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlMask.destAddr) != 0)
            {
                byte[] da = BitConverter.GetBytes(destinationAddress);
                Array.Reverse(da);
                foreach (byte b in da)
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlMask.seqNum) != 0)
            {
                byte[] sq = BitConverter.GetBytes(sequenceNum);
                Array.Reverse(sq);
                foreach (byte b in sq)
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlMask.ackBlock) != 0)
            {
                byte[] ak = BitConverter.GetBytes(acknowledgeBlock);
                Array.Reverse(ak);
                foreach (byte b in ak)
                    list.AddLast(b);
            }
            if ((controlField & (ushort)ControlMask.errorType) != 0)
            {
                list.AddLast(errorDetectType);

            }
            if ((controlField & (ushort)ControlMask.dataSize) != 0)
            {
                byte[] dl = BitConverter.GetBytes(dataLength);
                Array.Reverse(dl);
                foreach (byte b in dl)
                    list.AddLast(b);
            }

            byte[] byteArray = new byte[list.Count];

            LinkedListNode<byte> l = list.First;
            for (int i = 0; i < list.Count; i++)
            {
                byteArray[i] = l.Value;
                l = l.Next;
            }

            return byteArray;
        }

        /// <summary>
        /// Summary description for PacketHeader.
        /// </summary>
        public string toString()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendFormat("{0:X4}", startDelimeter);

            byte[] cf = BitConverter.GetBytes(controlField);
            sb.AppendFormat("{0:X2}", cf[1]);
            sb.AppendFormat("{0:X2}", cf[0]);

            if ((controlField & (ushort)ControlMask.linkState) != 0)
                sb.AppendFormat("{0:X2}", linkState);
            if ((controlField & (ushort)ControlMask.timeStamp) != 0)
                sb.AppendFormat("{0:X8}", timeStamp);
            if ((controlField & (ushort)ControlMask.asciiCallSign) != 0)
                sb.AppendFormat("{0:X16}", asciiCallSign);
            if ((controlField & (ushort)ControlMask.scrAddr) != 0)
                sb.AppendFormat("{0:X4}", sourceAddress);
            if ((controlField & (ushort)ControlMask.destAddr) != 0)
                sb.AppendFormat("{0:X4}", destinationAddress);
            if ((controlField & (ushort)ControlMask.seqNum) != 0)
                sb.AppendFormat("{0:X4}", sequenceNum);
            if ((controlField & (ushort)ControlMask.ackBlock) != 0)
                sb.AppendFormat("{0:X8}", acknowledgeBlock);
            if ((controlField & (ushort)ControlMask.errorType) != 0)
                sb.AppendFormat("{0:X2}", errorDetectType);
            if ((controlField & (ushort)ControlMask.dataSize) != 0)
                sb.AppendFormat("{0:X4}", dataLength);
            if (errorDetectType != 0x03)
                sb.AppendFormat("{0:X4}", crc);
            
            return sb.ToString();
        }


	}
}