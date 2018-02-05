/**********************************************************************************************************************
 *                                         Space Science & Engineering Lab - MSU
 *                                           Maia University Nanosat Program
 *
 *                                                    IMPLEMENTATION
 * Filename      : SessionEvents.cs
 * Programmer(s) : Chad Bohannan
 * Created       : 22 Aug, 2006
 * Description   : Session Events are the collected event objects used by the Session and Session Manager classes.
 **********************************************************************************************************************/
using System;

namespace ELP
{
    public class ConnectedArgs : EventArgs
    {
        public ushort dest;
        public ConnectedArgs(ushort dest) { this.dest = dest; }
    }
    public delegate void ConnectedHandler(object sender, ConnectedArgs e);

    public class DisconnectedArgs : EventArgs
    {
        public ushort dest;
        public DisconnectedArgs(ushort dest) { this.dest = dest; }
    }
    public delegate void DisconnectedHandler(object sender, DisconnectedArgs e);

    public class AcknowledgementArgs : EventArgs
    {
        public ushort dest;
        public AcknowledgementArgs(ushort dest) { this.dest = dest; }
    }
    public delegate void AcknowledgementHandler(object sender, AcknowledgementArgs e);

    public class DataRecievedArgs : EventArgs
    {
        public Packet packet;
        public ushort sourceAddress;
        public DataRecievedArgs(Packet packet, ushort source) 
        { 
            this.packet = packet; 
            this.sourceAddress = source; 
        }
    }
    public delegate void DataRecievedHandler(object sender, DataRecievedArgs e);

    public class RawPacketArgs : EventArgs
    {
        public Packet packet;
        public bool isValid;
        public RawPacketArgs(Packet packet, bool isValid) 
        { 
            this.packet = packet;
            this.isValid = isValid;
        }
    }
    public delegate void RawPacketHandler(object sender, RawPacketArgs e);
}