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
    public interface IPacketHandler
    {
        void OnPacket(Packet packet);
    }


    /// <summary>
    /// Parser class buffers content and maintains the state machine variables 
    /// </summary>
    public class Parser
    {
        #region Types
        // These are the states that the parser can be in.
        private enum State
        {
            FindStart,
            GetCF,
            GetHeader,
            GetData,
            GetCRC
        }
        
        #endregion

        #region Attrubutes

        private byte delimCount; // The current count of sequental delimiters.
        private State state; 
        byte[] buffer; // byte buffer a Packet is parsed from
        private ushort controlFlags;
        private int headerIndex;
        private int headerLength;
        private int dataIndex;
        private int dataLength;
        private int crcIndex;
        const int crcLength = 4;
        const int MaxHeaderSize = 19;
        const int MaxDataSize = 1 << 16;        
        const int MaxPacketSize = MaxHeaderSize + MaxDataSize + crcLength;

        IPacketHandler handler;

        #endregion

        public Parser(IPacketHandler handler)
        {
            this.handler = handler;
            buffer = new byte[MaxPacketSize];
            reset();
        }

        public void reset()
        {
            state = State.FindStart;
            headerIndex = 0;
            headerLength = 0;
            dataIndex = 0;
            dataLength = 0;
            crcIndex = 0;
        }

        public int readBytes(byte[] data)
        {
            foreach (byte b in data)
            {
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
                        goto default; // explicit fallthrough
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
            return data.Length;
        }

        /// <summary>
        /// This gets the Control Feild and calculates the total header size.
        /// </summary>
        /// <param name="b">The current byte being read.</param>
        private void GetCF(byte b)
        {
            buffer[headerIndex] = b;
            headerIndex += 1;
            if (headerIndex == 2)
            {
                controlFlags = Packet.readUINT16(buffer, 0);
                headerLength = Packet.headerLength(controlFlags);
                if (headerLength > 2)
                {
                    state = State.GetHeader;
                }
                else
                {
                    headerIndex = 0;
                    state = State.FindStart;
                }
            }           
        }

        /// <summary>
        /// This gets the Header and locates the Error Detection Type to find the CRC size.
        /// </summary>
        /// <param name="b">The current byte being worked on.</param>
        private void GetHeader(byte b)
        {
            buffer[headerIndex] = b;
            headerIndex += 1;
            if (headerIndex == headerLength)
            {
                if ((controlFlags & (ushort)Packet.ControlFlag.data) != 0)
                {
                    dataLength = Packet.readUINT16(buffer, headerIndex - 2);
                    dataIndex = 0;
                    state = State.GetData;
                }
                else if ((controlFlags & (ushort)Packet.ControlFlag.crc) != 0)
                {
                    dataLength = 0;
                    dataIndex = 0;
                    crcIndex = 0;
                    state = State.GetCRC;
                }
                else
                {
                    handler.OnPacket(new ELP.Packet(buffer));
                    reset();
                }
            }
        }

        /// <summary>
        /// This gets the body's data then sets the CRC size for the body.
        /// </summary>
        /// <param name="b">The current byte being worked on.</param>
        private void GetData(byte b)
        {
            int offset = headerIndex + dataIndex;
            buffer[offset] = b;
            dataIndex += 1;
            if (dataIndex == dataLength)
            {
                if ((controlFlags & (ushort)Packet.ControlFlag.crc) != 0)
                {
                    crcIndex = 0;
                    state = State.GetCRC;
                }
                else
                {
                    handler.OnPacket(new ELP.Packet(buffer));
                    reset();
                }
            }
        }

        /// <summary>
        /// This gets the CRC, checks the body then sends the package before reseting the parser.
        /// </summary>
        /// <param name="b">The current byte being worked on.</param>
        private void GetCRC(byte b)
        {
            int offset = headerIndex + dataIndex + crcIndex;
            buffer[offset] = b;
            crcIndex += 1;
            if (crcIndex == crcLength)
            {
                // TODO inspect CRC value

                handler.OnPacket(new ELP.Packet(buffer));
                reset();
            }
        }

    }// Parser class
}
