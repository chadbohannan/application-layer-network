/**********************************************************************************************************************
 *                                         Space Science & Engineering Lab - MSU
 *                                           Maia University Nanosat Program
 *
 *                                                    IMPLEMENTATION
 * Filename      : Packet.cs
 * Programmer(s) : Ehson Mosleh (ehson@ssel.montana.edu), Chad Bohannan
 * Created       : 22 Aug, 2006
 * Description   : The Packet in the SSEL Link Communication Protocol. This object includes a PacketHeader object, 
 *                  a byte array which is the data and a CRC for the data.
 **********************************************************************************************************************/
using System;
using System.Collections.Generic;
using System.Text;

namespace ELP
{
    /// <summary>
    /// Packet class definition. 
    /// </summary>
    public class Packet
    {
        //Header object for a instance of a pakcet objecr
        private PacketHeader header;

        //array of bytes which holds the Packet's data 
        private byte[] data;

        //crc for the data field
        private uint crc;

        //size of the packet
        private int size;

        #region ACCESSOR METHODS

        /// <summary>
        /// Get and Set the data byte array 
        /// </summary>
        public byte[] Data
        {
            get { return data; }
            set 
            { 
                data = value;
                if (data.Length > 65535)
                    throw new OverflowException("Data cannot be more than 65535 bytes.");
                this.header.DataLength = (ushort)data.Length;
                this.crc = PacketHeader.getCRC((PacketHeader.ErrorType)header.ErrorDetectType, data, (ushort)data.Length);
            }
        }

        /// <summary>
        /// Get and Set the data CRC.
        /// </summary>
        public uint Crc
        {
            get { return crc; }
            set { crc = value; }
        }

        public byte[] getCRCBytes()
        {
            byte[] crcBytes;
            if (PacketHeader.getCRCLength(header.ErrorDetectType) == 2)
                crcBytes = BitConverter.GetBytes((ushort)(0x0000FFFF & Crc));
            else if (PacketHeader.getCRCLength(header.ErrorDetectType) == 4)
                crcBytes = BitConverter.GetBytes(Crc);
            else
                crcBytes = new byte[0];
            Array.Reverse(crcBytes);
            return crcBytes;
        }
        /// <summary>
        /// Get and Set the Packet's header.
        /// </summary>
        public PacketHeader Header
        {
            get { return header; }
            set { header = value; }
        }

        #endregion ACCESSOR METHODS

        #region CONSTRUCTORS

        /// <summary>
        /// Default Constructor for the Packet class.
        /// </summary>
        public Packet()
        {
            this.header = new PacketHeader();
            data = new byte[0];
        }

        /// <summary>
        /// Copy Constructor.
        /// </summary>
        public Packet(Packet p)
        {
            this.Header = new PacketHeader(p.Header);
            if (p.header.HasData())
                this.data = (byte[])p.Data.Clone();
            else
                this.data = new byte[0];
            this.Crc = p.Crc;
        }

        #endregion CONSTRUCTORS

        #region UTILITY FUNCTIONS

        /// <summary>
        /// Sets the size of the data byte array. Also resets the size attribute 
        /// of the packet object.
        /// </summary>
        /// <param name="size">the size to give to the data byte array.</param>
        public void setDataBufferSize(ushort size)
        {
            this.data = new byte[size];
            this.size = size;
        }


        /// <summary>
        /// This adds a 0xC3 to the buffer after every three 0x3C,
        /// from offsetToIgnore to sizeToEnd. The buffer must start at zero
        /// and be sizeToEnd bytes long.
        /// </summary>
        /// <param name="buf">The beffer to start padding.</param>
        /// <param name="offsetToIgnore">The offset to start padding at.</param>
        /// <param name="sizeToEnd">The size of the input buffer.</param>
        /// <returns>The padded input buffer.</returns>
        private byte[] PadC3EveryThree3C(byte[] buf, int offsetToIgnore, int sizeToEnd)
        {
            int count = 0;
            System.Collections.ArrayList byteBuffer = new System.Collections.ArrayList();
            for (int i = 0; i < offsetToIgnore; i++)
                byteBuffer.Add(buf[i]);

            for (int i = offsetToIgnore; i < sizeToEnd; i++)
            {
                byteBuffer.Add(buf[i]);
                if (buf[i] == 0x3C)
                {
                    ++count;
                    if (count == 3)
                    {
                        byteBuffer.Add((byte)0xC3);
                        count = 0;
                    }
                }
                else count = 0;
            }
            byte[] returnBuf = new byte[byteBuffer.Count];
            for (int i = 0; i < byteBuffer.Count; i++)
            {
                returnBuf[i] = (byte)(byteBuffer[i]);
            }
            return returnBuf;
        }


        /// <summary>
        /// returns the packet object as a byte array 
        /// </summary>
        public byte[] toByteArray()
        {
            //use the PacketHeader.toByteArray() function to get the header 
            //minus the header CRC
            byte[] h = header.toByteArray();
            byte[] hCRC = header.getCRCBytes();
            int crcLength = hCRC.Length;
            byte[] dCRC = new byte[0];
            //pad the Data so that it is byte stuffed
            byte [] paddedData = PadC3EveryThree3C(data, 0, data.Length);

            byte[] result;
            if (header.HasData())
            {
                result = new byte[h.Length + paddedData.Length + 2 * crcLength];
                dCRC = getCRCBytes();
            }
            else
            {
                result = new byte[h.Length + crcLength];
            }

            int i = 0;

            //put every byte of the Header into the result
            for (int j = 0; j < h.Length; j++)
            {
                result[i] = h[j];
                i++;
            }
            
            //put the header crc in result
            for (int j = 0; j < crcLength; j++)
            {
                result[i] = hCRC[j];
                i++;
            }
            
            //put the data array and data CRC into the result
            if (header.HasData())
            {
                for (int j = 0; j < paddedData.Length; j++)
                {
                    result[i] = paddedData[j];
                    i++;
                }

                for (int j = 0; j < crcLength; j++)
                {
                    result[i] = dCRC[j];
                    i++;
                }
            }

            return result;
        }

        /// <summary>
        /// returns an human readable string of the Packet instance.
        /// </summary>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(this.Header.toString());
   
            return base.ToString();
        }

        #endregion UNTILITY FUNCTIONS

    }
}
