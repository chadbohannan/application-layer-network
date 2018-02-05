/**********************************************************************************************************************
 *                                         Space Science & Engineering Lab - MSU
 *                                           Maia University Nanosat Program
 *
 *                                                    IMPLEMENTATION
 * Filename      : Link.cs
 * Programmer(s) : Grant Nelson, Chad Bohannan
 * Created       : 22 Aug, 2006
 * Description   : Manages a connection over a channel using SSEL Packets.
 **********************************************************************************************************************/
using System.Collections;
using System.ComponentModel;
using System.Threading;
using System;

namespace ELP
{
    /// <summary>
    /// The Link class is used to managed the sending and recieving of packets over
    /// a communications channel. Currently only supports full duplex channels.
    /// </summary>
    public class Link
    {
        #region Types
        // These are the states that the parser can be in.
        private enum StateType
        {
            FindStart,
            GetCF,
            GetHeader,
            GetHeaderCRC,
            GetData,
            GetDataCRC
        }

        // ChannelType is determined by the constructor, and sets the communication
        // channel type.
        private enum ChannelType
        { 
            Serial,//RS-232 port
            Socket,//client TCP socket
            Server //TCPServer class that manages connections
        }
        #endregion

        #region Attrubutes
        Object transmitLock;
        System.IO.Ports.SerialPort ioport;
        System.Net.Sockets.Socket socket;
        TCPServer server;
        ChannelType channelType;

        public event RawPacketHandler RawPacketEvent;
        BackgroundWorker bw = new BackgroundWorker();
        private bool packetComplete;
        private bool packetIsValid;

        // These are defined sizes for buffers.
        private const int MaxHeaderSize = 37;
        private const int MaxDataBufferSize = 8192;

        // These are specific values picked up for later use by the parser.
        private ushort CFmsg;    // The Control Feild value.
        private byte Errmsg;     // The Error Detection Type value.
        private uint CRChdr;     // The current CRC for the header.
        private uint CRCmsg;     // The current CRC for the body.
        private byte delimCount; // The current count of sequental delimiters.

        // This is the current state of the parser.
        private StateType state;

        // This is the buffer and buffer values for the header.
        private byte headerIndex;
        private byte headerLength;
        private byte[] header;

        // This is the buffer and buffer values for the body.
        private ushort dataIndex;
        private ushort dataLength;
        private byte[] data;

        // This is the current packet being worked on.
        private Packet packet;
        private PacketHeader.ErrorType UpErr;
        #endregion

        #region Construction/Initilization
        public Link(System.IO.Ports.SerialPort ioport)
        {
            this.ioport = ioport;
            channelType = ChannelType.Serial;
            Initialize();
        }

        public Link(System.Net.Sockets.Socket socket)
        {
            this.socket = socket;
            channelType = ChannelType.Socket;
            Initialize();
        }

        public Link(TCPServer server)
        {
            this.server = server;
            channelType = ChannelType.Server;
            Initialize();
        }

        private void Initialize()
        {
            transmitLock = new Object();
            packetComplete = false;
            //the background worker is spending all it's time recieving packets
            bw.DoWork += new DoWorkEventHandler(bw_Recieve);
            bw.RunWorkerAsync();
        }
        #endregion

        #region Common Methods

        /// <summary>
        /// This CRC's a byte array with the given format.
        /// </summary>
        /// <param name="format">The format type of the CRC.</param>
        /// <param name="pdata">The byte array.</param>
        /// <param name="size">The length to CRC.</param>
        /// <returns>The CRC'ed value. (check sizing before writing this value)</returns>
        private uint getCRC(PacketHeader.ErrorType format, byte[] pdata, ushort size)
        {
            if (format == PacketHeader.ErrorType.NO_CRC) return 0x0000;
            byte c;
            ushort i, j;
            uint bit, crc;
            if (format == PacketHeader.ErrorType.CRC_CCITT)
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
            else if (format == PacketHeader.ErrorType.CRC16)
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

        /// <summary>
        /// This is a modified "Sparse One Bit Count".
        /// Sparse One is used beacuse there are 7 or less ones out of 16 bits, so
        /// it is best to look at the ones. Instead of counting the ones it just
        /// determines if there was an odd or even count of bits by XORing the int.
        /// </summary>
        /// <param name="n">This is the value to count the ones in.</param>
        /// <returns>0x0 for even or 0x1 for odd.</returns>
        private byte IntXOR(int n)
        {
            byte cnt = 0x0;
            while (n > 0)
            { /* This loop will only execute the number of times equal to the number of ones. */
                cnt ^= 0x1;
                n &= (n - 0x1);
            }
            return cnt;
        }

        /// <summary>
        /// This is a "Sparse One Bit Count".
        /// Sparse One is used beacuse there are 7 or less ones out of 16 bits, so
        /// it is best to look at the ones.
        /// </summary>
        /// <param name="n">This is the value to count the ones in.</param>
        /// <returns>The number of ones in the int.</returns>
        private byte IntCount(int n)
        {
            byte cnt = 0x0;
            while (n > 0)
            { /* This loop will only execute the number of times equal to the number of ones. */
                ++cnt;
                n &= (n - 0x1);
            }
            return cnt;
        }

        private ushort getShort(byte[] array, int index)
        {
            return (ushort)(((ushort)array[index] << 8) | (ushort)array[index + 1]);
        }

        private uint getInt(byte[] array, int index)
        {
            return (uint)(((uint)array[index] << 24) | ((uint)array[index + 1] << 16) | ((uint)array[index + 2] << 8) | (uint)array[index + 3]);
        }

        private ulong getLong(byte[] array, int index)
        {
            return (ulong)((ulong)array[index] << 56) | (ulong)((ulong)array[index + 1] << 48) | (ulong)((ulong)array[index + 2] << 40) | (ulong)((ulong)array[index + 3] << 32) |
                   (ulong)((ulong)array[index + 4] << 24) | (ulong)((ulong)array[index + 5] << 16) | (ulong)((ulong)array[index + 6] << 8) | (ulong)array[index + 7];
        }

        private void setShort(byte[] array, int index, ushort value)
        {
            array[index] = (byte)((value & 0xFF00) >> 8);
            array[index + 1] = (byte)(value & 0x00FF);
        }

        private void setInt(byte[] array, int index, uint value)
        {
            array[index] = (byte)((value & 0xFF000000) >> 24);
            array[index + 1] = (byte)((value & 0x00FF0000) >> 16);
            array[index + 2] = (byte)((value & 0x0000FF00) >> 8);
            array[index + 3] = (byte)(value & 0x000000FF);
        }

        private void setLong(byte[] array, int index, ulong value)
        {
            array[index] = (byte)((value & 0xFF00000000000000) >> 56);
            array[index + 1] = (byte)((value & 0x00FF000000000000) >> 48);
            array[index + 2] = (byte)((value & 0x0000FF0000000000) >> 40);
            array[index + 3] = (byte)((value & 0x000000FF00000000) >> 32);
            array[index + 4] = (byte)((value & 0x00000000FF000000) >> 24);
            array[index + 5] = (byte)((value & 0x0000000000FF0000) >> 16);
            array[index + 6] = (byte)((value & 0x000000000000FF00) >> 8);
            array[index + 7] = (byte)(value & 0x00000000000000FF);
        }

        #endregion

        #region I/O
        //if notConsumed is false, pendOnRead grabs a new byte from 
        //the ioport, blocking if nessesary
        private byte ReadByte()
        {
            byte value = 0;
            switch (channelType)
            { 
                case ChannelType.Serial:
                    value = ReadByteSerial();
                    break;
                case ChannelType.Socket:
                    value = ReadByteSocket();
                    break;
                case ChannelType.Server:
                    value = ReadByteServer();
                    break;
                default:
                    throw new Exception("Undefined channel type.");
            }
            return value;
        }

        private byte ReadByteSerial()
        {
            while (true)
            {
                try
                {
                    byte peeked = (byte)ioport.ReadByte();
                    return peeked;
                }
                catch { }
            }
        }

        private byte ReadByteSocket()
        { 
            byte[] datum = new byte[1];
            try
            {
                socket.Receive(datum);
            }
            catch { 
                //TODO:: shutdown this thread, and signal the closing of the link
                Thread.Sleep(10); 
            }
            return datum[0];
        }

        private byte ReadByteServer()
        {
            while (true)
                Thread.Sleep(1000);
        }

        public int Send(Packet packet)
        {
            int  retValue = 0;
            lock (transmitLock)
            {
                switch (channelType)
                {
                    case ChannelType.Serial:
                        retValue = SendSerial(packet);
                        break;
                    case ChannelType.Socket:
                        retValue = SendSocket(packet);
                        break;
                    case ChannelType.Server:
                        retValue = SendServer(packet);
                        break;
                    default:
                        throw new Exception("Undefined channel type.");
                }
            }
            return retValue;
        }

        private int SendSerial(Packet packet)
        {
            byte[] array = packet.toByteArray();
            if (ioport == null)
                return -1;
            ioport.Write(array, 0, array.Length);
            return 0;
        }

        private int SendSocket(Packet packet)
        {
            byte[] array = packet.toByteArray();
            if (socket == null)
                return -1;
            socket.Send(array);
            return 0;
        }

        private int SendServer(Packet packet)
        {
            if(packet.Header.HasSequenceNum())
                throw new Exception("Mode does not support sequence numbers.");
            byte[] array = packet.toByteArray();
            if (server == null)
                return -1;
            server.SendData(array);
            return 0;
        }
        #endregion

        #region Recieve Methods

        /// <summary>
        /// Fixes single bit errors in a Control Flag value.
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        ushort CFHamDecode(ushort value)
        {
            /* perform H matrix */
            ushort err = (ushort)(IntXOR(value & 0x826F)
              | (IntXOR(value & 0x41B7) << 1)
              | (IntXOR(value & 0x24DB) << 2)
              | (IntXOR(value & 0x171D) << 3));
            /* don't strip control flags, it will mess up the crc */
            switch (err) /* decode error field */
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

        /// <summary>
        /// This fills the Header of the package with data collected in the header buffer.
        /// </summary>
        private void FillOutHeader()
        {
            int index;

            //due to the behaviour of the packet accessor functions, the CF flag
            //is edited every time a header field is modified. As a result, we 
            //store the exact control flag, and assign explicitly at the end
            ushort CF = getShort(header, 4); // CF (2 bytes)

            index = 6; // 4 bytes (for delimiter) + 2 bytes (for cf).
            if ((CF & 0x0100) != 0x0000) //LinkState (1 byte)
            {
                packet.Header.LinkStateProp = header[index++];
            }
            if ((CF & 0x80) != 0x00) // TimeStamp  (4 bytes)
            {
                packet.Header.TimeStamp = getInt(header, index);
                index += 4;
            }
            if ((CF & 0x40) != 0x00) // Call Sign  (8 bytes)
            {
                byte[] asciiCallSign = new byte[8];
                for (int i = 0; i < 8; i++ )
                    asciiCallSign[0] = header[index + i];

                packet.Header.AsciiCallSign = asciiCallSign;
                index += 8;
            }
            if ((CF & 0x20) != 0x00) // ScrAddr    (2 bytes)
            {
                packet.Header.SourceAddress = getShort(header, index);
                index += 2;
            }
            if ((CF & 0x10) != 0x00) // DestAddr   (2 bytes)
            {
                packet.Header.DestinationAddress = getShort(header, index);
                index += 2;
            }
            if ((CF & 0x08) != 0x00) // SeqNum     (2 bytes)
            {
                packet.Header.SequenceNum = getShort(header, index);
                index += 2;
            }
            if ((CF & 0x04) != 0x00) // AckBlock   (4 bytes)
            {
                packet.Header.AcknowledgeBlock = getInt(header, index);
                index += 4;
            }
            if ((CF & 0x02) != 0x00) // Err        (1 byte)
            {
                packet.Header.ErrorDetectType = header[index];
                index++;
            }
            if ((CF & 0x01) != 0x00) // DataLength (2 bytes)
            {
                packet.Header.DataLength = getShort(header, index);
                index += 2;
            }
            packet.Header.Crc = CRChdr;
            //explicitly assign the control field at the end
            packet.Header.ControlField = CF;
        }

        /// <summary>
        /// This fills the package data with the data collected in the data buffer.
        /// </summary>
        private void FillOutData()
        {
            int length = packet.Header.DataLength;
            if (length < 1)
                return;
            if (length > MaxDataBufferSize)
                length = MaxDataBufferSize;//TODO:: handle error!!

            byte[] tempData = new byte[length];
            for (int i = 0; i < length; i++)
                tempData[i] = data[i];
            packet.Data = tempData;
            packet.Crc = CRCmsg;
        }
      

        /// <summary>
        /// This gets the Control Feild and calculates the total header size.
        /// </summary>
        /// <param name="msg">The current byte being worked on.</param>
        private void GetCF(byte msg)
        {
            if (headerIndex >= MaxHeaderSize)
            {   // Error, write out of bounds, reset

                packetIsValid = false;
                packetComplete = true;
                return;
            }
            header[headerIndex] = msg;
            headerIndex++;
            if (headerIndex == 6)
            {   // Ham CF
                CFmsg = getShort(header, 4);
                CFmsg = (ushort)CFHamDecode(CFmsg);
                setShort(header, 4, CFmsg);
                headerLength = 6;
                if ((CFmsg & 0x0100) != 0x00) headerLength += 1; // LinkState  (1 byte)
                if ((CFmsg & 0x0080) != 0x00) headerLength += 4; // TimeStamp  (4 bytes)
                if ((CFmsg & 0x0040) != 0x00) headerLength += 8; // Call Sign  (8 bytes)
                if ((CFmsg & 0x0020) != 0x00) headerLength += 2; // ScrAddr    (2 bytes)
                if ((CFmsg & 0x0010) != 0x00) headerLength += 2; // DestAddr   (2 bytes)
                if ((CFmsg & 0x0008) != 0x00) headerLength += 2; // SeqNum     (2 bytes)
                if ((CFmsg & 0x0004) != 0x00) headerLength += 4; // AckBlock   (4 bytes)
                if ((CFmsg & 0x0002) != 0x00) headerLength += 1; // Err        (1 byte)
                if ((CFmsg & 0x0001) != 0x00) headerLength += 2; // DataLength (2 bytes)
                state = StateType.GetHeader;
            }
        }

        /// <summary>
        /// This gets the Header and locates the Error Detection Type to find the CRC size.
        /// </summary>
        /// <param name="msg">The current byte being worked on.</param>
        private void GetHeader(byte msg)
        {
            //error handling for garbage detection
            if (headerIndex >= MaxHeaderSize)
            {   // Error, write out of bounds, reset
                packetIsValid = false;
                packetComplete = true;
                return;
            }
            header[headerIndex] = msg;
            headerIndex++;
            if (headerIndex >= headerLength)
            {   // read error type
                if ((CFmsg & (ushort)PacketHeader.ControlMask.errorType) == 0x00) 
                    Errmsg = (byte)PacketHeader.ErrorType.NO_CRC;
                else if ((CFmsg & (ushort)PacketHeader.ControlMask.dataSize) != 0x00) 
                    Errmsg = header[headerLength - 3];
                else Errmsg = header[headerLength - 1];
                // Error check Errmsg
                if (IntCount(Errmsg ^ (byte)PacketHeader.ErrorType.NO_CRC) <= 2)
                {
                    Errmsg = (byte)PacketHeader.ErrorType.NO_CRC;
                    headerLength += 0;
                    UpErr = PacketHeader.ErrorType.NO_CRC;
                    if ((CFmsg & (ushort)PacketHeader.ControlMask.dataSize) != 0x00)
                    {
                        //compute data length here
                        dataLength = getShort(header, headerIndex - 2);
                        dataIndex = 0;
                        state = StateType.GetData;
                    }
                    else
                    {
                        packetIsValid = true;
                        packetComplete = true;
                        return;
                    }
                }
                else if (IntCount(Errmsg ^ (byte)PacketHeader.ErrorType.CRC16) <= 2)
                {
                    Errmsg = (byte)PacketHeader.ErrorType.CRC16;
                    headerLength += 2;
                    UpErr = PacketHeader.ErrorType.CRC16;
                    state = StateType.GetHeaderCRC;
                }
                else if (IntCount(Errmsg ^ (byte)PacketHeader.ErrorType.CRC_CCITT) <= 2)
                {
                    Errmsg = (byte)PacketHeader.ErrorType.CRC_CCITT;
                    headerLength += 2;
                    UpErr = PacketHeader.ErrorType.CRC_CCITT;
                    state = StateType.GetHeaderCRC;
                }
                else if (IntCount(Errmsg ^ (byte)PacketHeader.ErrorType.CRC32) <= 2)
                {
                    Errmsg = (byte)PacketHeader.ErrorType.CRC32;
                    headerLength += 4;
                    UpErr = PacketHeader.ErrorType.CRC32;
                    state = StateType.GetHeaderCRC;
                }
                else // Bad error code, reset
                {
                    packetIsValid = false;
                    packetComplete = true;
                    return;
                }
            }
        }

        /// <summary>
        /// This gets the Header's CRC checks header then gets the body size.
        /// </summary>
        /// <param name="msg">The current byte being worked on.</param>
        private void GetHeaderCRC(byte msg)
        {
            if (headerIndex >= MaxHeaderSize)
            {   // Error, write out of bounds
                packetIsValid = false;
                packetComplete = true;
                return;
            }
            header[headerIndex] = msg;
            headerIndex++;
            if (headerIndex >= headerLength)
            {
                int length = headerIndex;
                PacketHeader.ErrorType errorType = (PacketHeader.ErrorType) Errmsg;
                if (Errmsg == (byte)PacketHeader.ErrorType.NO_CRC)
                {
                    /* Do Nothing, continue */
                }
                else if ((Errmsg == (byte)PacketHeader.ErrorType.CRC16) || 
                         (Errmsg == (byte)PacketHeader.ErrorType.CRC_CCITT))
                {
                    length = headerIndex - 2;
                    CRChdr = (ushort)PacketHeader.getCRC(errorType, header, (ushort)length);
                    ushort temp = getShort(header, length);
                    if (CRChdr != getShort(header, length))
                    {   // bad CRC, reset
                        packetIsValid = false;
                        packetComplete = true;
                        return;
                    }
                }
                else if (Errmsg == (byte)PacketHeader.ErrorType.CRC32)
                {
                    length = headerIndex - 4;
                    byte[] tempHeader = new byte[length];
                    for (int i = 0; i < length; i++)
                        tempHeader[i] = header[i];
                    CRChdr = (uint)PacketHeader.getCRC(errorType, tempHeader, (ushort)length);
                    if (CRChdr != getInt(header, length))
                    {   // bad CRC, reset
                        packetIsValid = false;
                        packetComplete = true;
                        return;
                    }
                }
                if ((CFmsg & (ushort)PacketHeader.ControlMask.dataSize) == 0x00)
                {   // blank header with no body (floating head), send then reset
                    FillOutHeader();
                    packetIsValid = true;
                    packetComplete = true;
                    return;
                }
                else
                {   // get body's length.
                    dataLength = getShort(header, length - 2);
                    if (dataLength <= 0)
                    {   // blank header with no body (floating head), send then reset
                        FillOutHeader();
                        packetIsValid = true;
                        packetComplete = true;
                        return;
                    }
                }
                // setting up Package
                if (dataLength > MaxDataBufferSize)
                {   // data length too large
                    packetIsValid = false;
                    packetComplete = true;
                    return;
                }
                dataIndex = 0;
                state = StateType.GetData;
            }
        }

        /// <summary>
        /// This gets the body's data then sets the CRC size for the body.
        /// </summary>
        /// <param name="msg">The current byte being worked on.</param>
        private void GetData(byte msg)
        {
            if (dataIndex > MaxDataBufferSize)
            {
                packetIsValid = false;
                packetComplete = true;
                return;
            }
            data[dataIndex] = msg;
            dataIndex++;
            if (dataIndex >= dataLength)
            {   // Data filled, CRC the data
                if ((Errmsg == (byte)PacketHeader.ErrorType.NO_CRC))
                {
                    packetIsValid = true;
                    packetComplete = true;
                }
                if ((Errmsg == (byte)PacketHeader.ErrorType.CRC16) || 
                    (Errmsg == (byte)PacketHeader.ErrorType.CRC16))
                {
                    dataLength += 2;
                    state = StateType.GetDataCRC;
                }
                else if (Errmsg == (byte)PacketHeader.ErrorType.CRC32)
                {
                    dataLength += 4;
                    state = StateType.GetDataCRC;
                }
            }
        }

        /// <summary>
        /// This gets the CRC, checks the body then sends the package before reseting the parser.
        /// </summary>
        /// <param name="msg">The current byte being worked on.</param>
        private void GetDataCRC(byte msg)
        {
            data[dataIndex] = msg;
            dataIndex++;
            /* Don't Consume Byte Here */
            if (dataIndex >= dataLength)
            {
                int length;
                uint originalCRC;
                if (Errmsg == (byte)PacketHeader.ErrorType.NO_CRC)
                {
                    packetIsValid = true;
                    packetComplete = true;
                    return;
                }
                if ((Errmsg == (byte)PacketHeader.ErrorType.CRC16) || 
                    (Errmsg == (byte)PacketHeader.ErrorType.CRC_CCITT))
                {
                    length = dataIndex - 2;
                    originalCRC = getShort(data, length);
                }
                else //if (Errmsg == (byte)PacketHeader.ErrorType.CRC32)
                {
                    length = dataIndex - 4;
                    originalCRC = getInt(data, length);
                }

                CRCmsg = (uint)PacketHeader.getCRC(UpErr, data, (ushort)length);
                if (CRCmsg == originalCRC)
                    packetIsValid = true;
                else
                    packetIsValid = false;

                packetComplete = true;
            }
        }

        /// <summary>
        /// Background task function that parses incoming packets
        /// </summary>
        /// <param name="sender"></param>
        protected void bw_Recieve(object s, DoWorkEventArgs e)
        {
            // Set default values.
            this.delimCount = 0;
            this.state = StateType.FindStart;
            this.headerIndex = 0;
            this.dataIndex = 0;

            // Gather data storage.
            this.header = new byte[MaxHeaderSize];
            this.data = new byte[MaxDataBufferSize];
            this.packet = new Packet();

            while (true)
            {
                byte msg = ReadByte();

                // check for escape char 0xC3
                if ((delimCount == 3) && (msg == 0xC3))
                {   // reset delim count and don't send on char
                    delimCount = 0;
                    continue;
                }
                // check for start delim, '<' (0x3C)
                else if (msg == 0x3C)
                {
                    delimCount++;
                    if (delimCount == 4)
                    {   // reset system, start delim found
                        header[0] = 0x3C;
                        header[1] = 0x3C;
                        header[2] = 0x3C;
                        header[3] = 0x3C;
                        headerIndex = 4;
                        state = StateType.GetCF;
                        continue;
                    }
                }
                // reset delim count
                else delimCount = 0;

                // use current char in following state
                switch (state)
                {
                    case StateType.FindStart: break;
                    case StateType.GetCF: GetCF(msg); break;
                    case StateType.GetHeader: GetHeader(msg); break;
                    case StateType.GetHeaderCRC: GetHeaderCRC(msg); break;
                    case StateType.GetData: GetData(msg); break;
                    case StateType.GetDataCRC: GetDataCRC(msg); break;
                }

                if (packetComplete == true)
                {
                    FillOutHeader();
                    FillOutData();
                    //fire the new packet recieved event
                    RawPacketEvent(this, new RawPacketArgs(packet, packetIsValid));
                    //reset the incoming packet buffer
                    this.packet = new Packet();
                    //reset the parser state
                    state = StateType.FindStart;
                    packetComplete = false;
                }
            }
        }

        #endregion UpLink Methods
    }//Link class
}
