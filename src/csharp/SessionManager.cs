/**********************************************************************************************************************
 *                                         Space Science & Engineering Lab - MSU
 *                                           Maia University Nanosat Program
 *
 *                                                    IMPLEMENTATION
 * Filename      : PacketHeader.cs
 * Programmer(s) : Chad Bohannan
 * Created       : 22 Aug, 2006
 * Description   : The Session Manager maintains  a set of Sessions. This makes it possible for a single node to 
 *                  communicate with persistant links to multiple nodes simultaniously.
 **********************************************************************************************************************/
using System;
using System.IO.Ports;
namespace ELP
{
    /// <summary>
    /// A Session Manager contains several sessions, and manages their use of a COM port
    /// </summary>
    public class SessionManager
    {
        private ushort src_address; //the local machine address doesn't change
        System.Collections.Hashtable sessions;
        Link link;

        //when a connection is established, this event is fired
        public event ConnectedHandler ConnectedEvent;

        //when a connection fails to be established, or is dropped, this event is fired
        public event DisconnectedHandler DisconnectedEvent;

        //fired when a DATAGRAM sends successfully, or a STREAM packet is acknowledged
        public event AcknowledgementHandler AcknowledgementEvent;

        public event DataRecievedHandler DataRecievedEvent;

        public event RawPacketHandler RawPacketEvent;

        int maxPacketSize;

        public SessionManager(SerialPort ioport, ushort srcAdress)
        {
            src_address = srcAdress;
            sessions = new System.Collections.Hashtable();
            Open(ioport);
        }

        private void AddSession(ushort dest)
        {
            sessions.Add(dest, new Session(link, src_address, dest, maxPacketSize));
            ((Session)sessions[dest]).ConnectedEvent += new ConnectedHandler(Session_ConnectedEvent);
            ((Session)sessions[dest]).DisconnectedEvent += new DisconnectedHandler(Session_DisconnectedEvent);
            ((Session)sessions[dest]).AcknowledgementEvent += new AcknowledgementHandler(Session_AcknowledgementEvent);
            ((Session)sessions[dest]).DataRecievedEvent += new DataRecievedHandler(SessionManager_DataRecievedEvent);
        }

        private int Open(System.IO.Ports.SerialPort ioport)
        {
            link = new Link(ioport);
            int baudrate = ioport.BaudRate;
            maxPacketSize = baudrate / 4;
            if (maxPacketSize > 4096)
                maxPacketSize = 4096;
            link.RawPacketEvent += new RawPacketHandler(link_RawPacketEvent);
            return 0;
        }


        //send a byte array to a particular destination
        public int Send(ushort dest, byte[] data, bool isStream)
        {
            if (sessions.Contains(dest))
            {
                return ((Session)sessions[dest]).Send(data, isStream);
            }
            else //sessions does not contain the destination
            {
                return -1;
            }
        }

        //uses the dest parameter to route the function call to the appropriate session
        public int Connect(ushort dest, System.DateTime timeout)
        {
            if (sessions.Contains(dest) == false)
                AddSession(dest);
            return ((Session)sessions[dest]).Connect(timeout);
        }

        //direct particular session to disconnect
        public int Disconnect(ushort dest)
        {
            if (sessions.Contains(dest))
            {
                return ((Session)sessions[dest]).Disconnect();
            }
            else //sessions does not contain the destination
            {
                return -1;
            }
        }

        //direct a session to send a close message, which the results in a disconnect
        public int Close(ushort dest)
        {
            if (sessions.Contains(dest))
            {
                return ((Session)sessions[dest]).Close();
            }
            else //sessions does not contain the destination
            {
                return -1;
            }
        }

        void link_RawPacketEvent(object sender, RawPacketArgs e)
        {
            try
            {
                //always try to pass the packet event upwards
                if (RawPacketEvent != null)
                    RawPacketEvent(this, new RawPacketArgs(new Packet(e.packet), e.isValid));                //attempt to process valid packets by the associated session

                //allow the associated session to handle the incomming message
                //create a new session if nessesary
                if (e.isValid)
                {
                    ushort remoteAddress = e.packet.Header.SourceAddress;
                    if (sessions.Contains(remoteAddress) == false)
                        AddSession(remoteAddress);
                    ((Session)sessions[remoteAddress]).Recieve(new Packet(e.packet));
                }
            }
            catch (Exception exception)
            {
                //throw new Exception("Raw Packet Event", exception);
            }
        }

        void Session_AcknowledgementEvent(object sender, AcknowledgementArgs e)
        {
            if(AcknowledgementEvent != null)
                AcknowledgementEvent(this, e);
        }

        void Session_DisconnectedEvent(object sender, DisconnectedArgs e)
        {
            if (DisconnectedEvent != null)
                DisconnectedEvent(this, e);
        }

        void Session_ConnectedEvent(object sender, ConnectedArgs e)
        {
            if(ConnectedEvent != null)
                ConnectedEvent(this, e);
        }

        void SessionManager_DataRecievedEvent(object sender, DataRecievedArgs e)
        {
            if (DataRecievedEvent != null)
                DataRecievedEvent(this, new DataRecievedArgs(new Packet(e.packet), e.sourceAddress));
        }

    }//class SessionManager
}//namespace SessionManagement 
