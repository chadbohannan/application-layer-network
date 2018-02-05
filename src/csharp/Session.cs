/**********************************************************************************************************************
 *                                         Space Science & Engineering Lab - MSU
 *                                           Maia University Nanosat Program
 *
 *                                                    IMPLEMENTATION
 * Filename      : Session.cs
 * Programmer(s) : Chad Bohannan
 * Created       : 22 Aug, 2006
 * Description   : A session manages the persistance of an SSEL Protocol channel. This include connection, packet 
 *                  ordering, and detecting link failure.
 **********************************************************************************************************************/
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System;

namespace ELP
{
    /// <summary>
    /// A session is responsible for maintaining all state information and history for a connection
    /// </summary>
    public class Session
    {
        #region Constants
        //TODO:: load all constants from a configuration file
        int PACKETSIZE = 2048;//max data size in bytes
        int RESEND_TIMEOUT_SECONDS = 4;
        enum Status : byte
        {
            connecting,
            connected,
            disconnected
        };
        #endregion
        #region Internal Classes

        //SendChacheItem: used to cache data while packets are sent
        internal class SendChacheItem
        {
            public byte[] data;
            public bool isStream;
            public SendChacheItem(byte[] data, bool isStream)
            { this.isStream = isStream; this.data = data; }
        }

        internal class ResendItem
        {
            public Packet packet;
            public DateTime resendTime;
            public int resendCount;
            public ResendItem(Packet packet, DateTime resendTime)
            { this.packet = packet; this.resendTime = resendTime; resendCount = 0; }
        }
        
        internal class SequenceNumberComparer : IComparer
        {
            public int Compare(object x, object y)
            {
                int X = System.Convert.ToInt32(x);
                int Y = System.Convert.ToInt32(y);
                
                if(X > 32000+Y)
                    return Y - X;
                else
                    return X - Y; 
            }
        }

        #endregion
        #region Attributes
        private Status status;//defines the state of the session
        private Object statusLock = new Object();//mutex object for locking the status
        protected Link link; //the external like device this session transmits on
        protected ushort srcAddress; //this sessions 16 bit address
        protected ushort destAddress; //the address of the host this sessio is talking to
        protected Packet packetTemplate;//this session's default packet construction 
        private ushort nextSeqNum;//the next outgoing sequence number (not yet used)
        private ushort expectedSeqNum;//the next incomming sequence number for the stream

        private DateTime lastActiveTime;
        private Object timeLock = new Object();

        //bw is responsible for waiting on the link.Send() call
        BackgroundWorker bw = new BackgroundWorker();

        //the sendCache contains SendCacheItems that are going to be transmitted soon
        Queue<SendChacheItem> sendCache = new Queue<SendChacheItem>();

        //the retransmit buffer contains only STREAM type packets, with sequence numbers
        Queue<ResendItem> retransmitWindow = new Queue<ResendItem>();

        //The receive window contains messages that are not yet sequencable due to missing packets
        Queue<Packet> receiveWindow = new Queue<Packet>();
        
        //the ackBuffer contains unacknowldedge sequence numbers
        //these are numbers that came from the target, and need to be placed in AckBlocks
        ArrayList ackBuffer = new ArrayList(17);

        //when a connection is established, this event is fired
        public event ConnectedHandler ConnectedEvent;

        //when a connection fails to be established, or is dropped, this event is fired
        public event DisconnectedHandler DisconnectedEvent;

        //fired when a DATAGRAM sends successfully, or a STREAM packet is acknowledged
        public event AcknowledgementHandler AcknowledgementEvent;

        //fired when a packet with usefull data is recieved
        public event DataRecievedHandler DataRecievedEvent;

        #endregion 
        #region Constructors
        public Session(Link link, ushort src, ushort dest, int maxPacketSize)
        {
            status = Session.Status.disconnected;
            InitializeBackgoundWorker();
            this.link = link;
            srcAddress = src;
            destAddress = dest;

            //a template makes creating new packets simpler
            packetTemplate = new Packet();
            packetTemplate.Header.SourceAddress = srcAddress;
            packetTemplate.Header.DestinationAddress = destAddress;
            packetTemplate.Header.ErrorDetectType = (byte)PacketHeader.ErrorType.CRC16;

            //nextSeqNum = 65516;//2^16 - 20
            nextSeqNum = 0;
            
            lastActiveTime = DateTime.Now;
            PACKETSIZE = maxPacketSize;
        //debuging and analysis OVERRIDE
            //PACKETSIZE = 200;
        }

        //configure the transmit thread
        private void InitializeBackgoundWorker()
        {
            bw.DoWork += new DoWorkEventHandler(bw_SendData);
            bw.RunWorkerAsync();
        }
        #endregion
        #region Recieve Methods
        //Initiate periodic CONNECT packets for this session
        public int Connect(System.DateTime connect_timeout)
        {
            if (isConnected())
                return -1;
            else
            {
                lock(statusLock)
                    status = Session.Status.connecting;
            }
            return 0;
        }

        //sends a CLOSE message, then disconnects
        public int Close()
        {
            lock (statusLock)
            {
                if (status == Status.connected)
                {
                    Packet p = new Packet(packetTemplate);
                    p.Header.LinkStateProp = (byte)PacketHeader.LinkState.CLOSE;
                    SendPacket(p);
                }
            }
            
            return this.Disconnect();
        }

        //Stop all communications, flush the buffers and reset the windows
        //by informing the transmit thread (backgroundworker)
        public int Disconnect()
        {
            int retVal = 0;
            lock (statusLock)
            {
                if (status != Status.disconnected)
                {
                     //by setting the status, the bw_SendData function will clear all the buffers
                    status = Status.disconnected;
                }
                else
                    retVal = -1;
            }

            if ( retVal == 0 && DisconnectedEvent != null)
                DisconnectedEvent(this, new DisconnectedArgs(destAddress));
            return retVal;
        }

        public bool isConnected()
        {
            bool retVal;
            lock (statusLock)
            {
                if (status == Status.connected)
                    retVal = true;
                else
                    retVal = false;
            }
            return retVal;
        }

        //Recieve is called by the Session Manager to allow the session
        //to process acknowledgements and Link State transitions
        public int Recieve(Packet packet)
        {
            if(packet.Header.HasLinkStateProp())
                HandleLinkState(packet);

            //if a packet has data, it may or may not have a sequence number
            //sequence numbers are handled inside the HandleData() routine
            if (packet.Header.HasData())
                HandleData(packet);

            //TODO:: check for acknowledgements -> raise event, clear from retransmit window
            if (packet.Header.HasAcknowledgeBlock())
                HandleAcknowledgments(packet);

            //deliver data in order by firing events
            FlushRecieveQueue();

            //TODO:: check for ACKRESEND request
            return 0;
        }

        //recieved packets with a LinkStateProp field are processed here
        private int HandleLinkState(Packet packet)
        {
            lock (timeLock)//reset the 'something happend' clock right away
                lastActiveTime = DateTime.Now;

            //PONDER: if a packet says CONNECT, and we are already connected, what do we do?
            //if we are not connected, send a CONNECTED reply, chanage state, and fire event.
            if (packet.Header.LinkStateProp == (byte)PacketHeader.LinkState.CONNECT)
            {
                //connect packets allow the sender to define/redefine the sequence number
                //PROBLEM: data in the recieve window may not get sent up after redefinintion
                //SOLUTION? flush the recieve queue in sequence order, and allow gaps
                //SOLUTION? drop all the data in the recieve window
                expectedSeqNum = (ushort)(packet.Header.SequenceNum + 1);
                Packet p = new Packet(packetTemplate);
                p.Header.LinkStateProp = (byte)PacketHeader.LinkState.CONNECTED;
                p.Header.SequenceNum = nextSeqNum++;
                //we want to acknowedge the new sequence number from the target
                QueueAcknowledgment(packet.Header.SequenceNum);
                //this is where we actually attach the acknowledgment to an outgoing packet
                AddAcknowledgments(ref p);
                SendPacket(p);

                lock (statusLock)
                {
                    if (status != Status.connected)
                    {
                        status = Status.connected;
                        if (ConnectedEvent != null)
                            ConnectedEvent(this, new ConnectedArgs(packet.Header.SourceAddress));
                    }
                }
            }

            //if this node sent a CONNECT message, do not set state to connected until 
            // recieving a CONNECTED reply
            if (packet.Header.LinkStateProp == (byte)PacketHeader.LinkState.CONNECTED)
            {
                //update the expected sequence number for messeages from the remote host
                expectedSeqNum = (ushort)(packet.Header.SequenceNum + 1);
                //store the sequence number for later acknowledgment
                ackBuffer.Add((Int16)packet.Header.SequenceNum);
                lock (statusLock)
                    if (status != Status.connected)
                    {
                        status = Status.connected;
                        if (ConnectedEvent != null)
                            ConnectedEvent(this, new ConnectedArgs(packet.Header.SourceAddress));
                    }
            }

            //we we get a CLOSE message, there is no reply, shut down everything
            if (packet.Header.LinkStateProp == (byte)PacketHeader.LinkState.CLOSE)
            {
                this.Disconnect();
            }
            return 0;
        }

        /// <summary>
        /// Create an Arraylist of acknowledged blocks from an Acknowledgeblock field.
        /// </summary>
        /// The acknowledge block field is a 16 bit base value followed by 16 offset bits.
        /// <param name="ackBlock"></param>
        /// <returns></returns>
        private ArrayList GetArrayListFromAckBlock(uint ackBlock)
        {
            ArrayList acks = new ArrayList();
            uint baseAck = (ackBlock & (uint)0xFFFF0000) >> 16;
            uint bitField = (ackBlock & (uint)0x0000FFFF);
            acks.Add(baseAck);
            for(int i=0; i<16; i++)
            {
                ++baseAck;
                if ((bitField & 0x00008000) != 0)
                    acks.Add(baseAck);
                bitField = bitField << 1;
            }
            return acks;
        }

        //Use the acknowledgment field to clean the resend queue
        private int HandleAcknowledgments(Packet packet)
        {
            //cycle through the entire window, re-Enqueueing un-acked messages
            //retransmitWindow is still chronologially ordered by expiration time.
            lock (retransmitWindow)
            {
                int iters = retransmitWindow.Count;
                ArrayList acks = GetArrayListFromAckBlock(packet.Header.AcknowledgeBlock);
                lock (retransmitWindow)
                    for (int i = 0; i < iters; i++)
                    {
                        //remove the first element in the resend queue
                        ResendItem item = retransmitWindow.Dequeue();
                        //if that packet has been acknowledged, do not reenqueue it
                        if (!acks.Contains((uint)item.packet.Header.SequenceNum))
                            retransmitWindow.Enqueue(item);//only reenqueue nonmatching items
                    }
            }
            return 0;
        }

        //manage the interleaving of datagrams and sequenced packets
        //datagrams are buffered until there are no sequenced packets buffered 
        private int HandleData(Packet packet)
        {
            if (packet.Header.HasSequenceNum())
                HandleSequenceNumber(packet);
            else
                DataRecievedEvent(this, new DataRecievedArgs(packet, this.destAddress));
            return 0;
        }

        //process sequence numbers so that data events occur in order, and acks are made
        private int HandleSequenceNumber(Packet packet)
        {
            //only recieve the packet if the sequence number is equal to or greater 
            //than the current expected sequence number. acknowledge but do not recieve others
            //handle wraparound sequence numbers by numbers that are far apart
            if (packet.Header.SequenceNum >= expectedSeqNum ||
                packet.Header.SequenceNum < expectedSeqNum - 32000)
                RecieveQueueAddUnique(packet);

            //always queue acknowledgments
            QueueAcknowledgment(packet.Header.SequenceNum);
            return 0;
        }

        //searches for a matching sequence number. if one is not found, the packet is inserted
        private void RecieveQueueAddUnique(Packet packet)
        {
            lock (receiveWindow)
            {
                foreach (Packet p in receiveWindow)
                {
                    if (p.Header.SequenceNum == packet.Header.SequenceNum)
                        return;
                }
                receiveWindow.Enqueue(packet);
            }
        }

        //deliver data in order by checking to the current expected sequence number
        private void FlushRecieveQueue()
        {
            //it is possible that packets have been queued behind the newly recieved packet
            //we must flush out packets that can be sequenced correctly
            //we need to scan the entire recieve window for matching packets, it is unordered
            lock (receiveWindow)
            {
                int count = receiveWindow.Count;
                for (int i = 0; i <= count; i++)
                {
                    Packet checkPacket = receiveWindow.Dequeue();
                    if (checkPacket.Header.SequenceNum == expectedSeqNum)
                    {
                        ++expectedSeqNum;//expect the next seqNum (ushort rolls over automatically)
                        DataRecievedEvent(this, new DataRecievedArgs(checkPacket, this.destAddress));
                        i = 0; count--; continue;//restart the search for every success
                    }
                    else
                        receiveWindow.Enqueue(checkPacket);//not ready, put it back
                }
            }
        }

        //packets recieved with sequence numbers need to be acknowledged soon
        //this function agregates and sorts sequence numbers for pending acknowledgements
        private void QueueAcknowledgment(int number)
        {
            SequenceNumberComparer cmp = new SequenceNumberComparer();
            try
            {
                lock (ackBuffer)
                {
                    //check for membership, do not double insert values
                    if (!ackBuffer.Contains(number))
                    {
                        ackBuffer.Add(number);
                        ackBuffer.Sort(cmp);//make the lowest number first, ect
                    }
                }
            }
            catch (Exception e)
            {
                throw new Exception("QueAcknowldment failed", e);
            }
        }
        #endregion
        #region Send Methods

        //Package data into a Packet Class, then send the data through Link.
        //return value is the number of packets the message was segmented into. 
        //negative values are errors.
        public int Send(byte[] data, bool isStream)
        {
            if (status == Status.disconnected)
                return -1;
            else
                lock (sendCache)
                {
                    //TODO:: logically break up the input data into chunks
                    if (data.Length > PACKETSIZE)
                    {
                        int offset = 0;
                        while (offset < data.Length)
                        {
                            byte[] segData;
                            if ((data.Length - offset) > PACKETSIZE)
                                segData = new byte[PACKETSIZE];
                            else
                                segData = new byte[data.Length - offset];
                            for (int i = 0; i < segData.Length; i++)
                                segData[i] = data[offset + i];
                            offset += segData.Length;
                            sendCache.Enqueue(new SendChacheItem(segData, true));
                        }
                    }
                    else
                        sendCache.Enqueue(new SendChacheItem(data, isStream));
                }//end lock
            return 0;
        }

        //If there are any acknowledgments ready to go, add them to the packet
        private bool AddAcknowledgments(ref Packet packet)
        {
            ushort baseSequenceNumber;
            uint acknowledgeBlock;
            lock (ackBuffer)
            {
                if (ackBuffer.Count > 0)
                {   //the first ack is an explicit value
                    baseSequenceNumber = (ushort)(System.Convert.ToInt32(ackBuffer[0]));
                    acknowledgeBlock = ((uint)baseSequenceNumber) << 16;
                    ackBuffer.RemoveAt(0);
                
                    while (ackBuffer.Count > 0)
                    {   //set bits in the acknowledge block for subsequent acks
                        ushort offsetSequenceNumber = (ushort)(System.Convert.ToInt32(ackBuffer[0]));
                        ackBuffer.RemoveAt(0);
                        if (offsetSequenceNumber < baseSequenceNumber + 16)
                        {
                            int offset = 16 - (offsetSequenceNumber - baseSequenceNumber);
                            acknowledgeBlock += (uint)( 0x0001 << offset);
                        }
                        else
                            break;
                    }
                    packet.Header.AcknowledgeBlock = acknowledgeBlock;
                }
            }
            return false;
        }

        //checks the resendWindow structure for expired packets, and attemps the resend
        private bool ResendData()
        {
            //this method only checks the first element for and expired time
            //if the first element is expired, it is Dequeued, resent, and requeued
            //This gaurentees temporal ordering of the resend queue, such that only
            //the first element ever needs to be checked.
            bool dataResent;
            lock (retransmitWindow)
            {
                if (retransmitWindow.Count < 1)
                    dataResent = false;
                else if (retransmitWindow.Peek().resendTime <= DateTime.Now)
                {
                    //TODO:: freak out over excessive resend counts
                    ResendItem resendItem = retransmitWindow.Dequeue();
            int seqNumDEBUG = resendItem.packet.Header.SequenceNum;
                    //we are overridding any old ackblocks that were applied before
                    AddAcknowledgments(ref resendItem.packet);//pack any available acknowldgments
                    SendPacket(resendItem.packet);

                    resendItem.resendCount++;
                    resendItem.resendTime = DateTime.Now + TimeSpan.FromSeconds(4);
                    retransmitWindow.Enqueue(resendItem);
                    dataResent = true; //there was a message resnd (link.Send was called)
                }
                else
                    dataResent = false; //there were no expired messages to resend
            }
            return dataResent;
        }

        //Checks the sendCache for any data that is ready to send. If
        private bool SendCachedData()
        {
            SendChacheItem sendItem = null;
            lock (this.sendCache)
                if (sendCache.Count > 0)
                    sendItem = sendCache.Dequeue();
            if (sendItem != null)
            {
                Packet dataPacket = new Packet(packetTemplate);//default src/dest
                dataPacket.Data = sendItem.data;//load the data
                if (sendItem.isStream)
                {//sequence stream items and put them in the resend queue
                    dataPacket.Header.SequenceNum = nextSeqNum++;
                    DateTime resendTime = DateTime.Now + TimeSpan.FromSeconds(RESEND_TIMEOUT_SECONDS);
                    lock (retransmitWindow)
                        retransmitWindow.Enqueue(new ResendItem(dataPacket, resendTime));
                }
                //resends should get get new ackblocks
                AddAcknowledgments(ref dataPacket);//pack any available acknowldgments
                SendPacket(dataPacket);//transmit the packet
                sendItem = null;//effectivly delete packets not in resend
                return true;
            }
            else
                return false;
        }

        private void SendPacket(Packet packet)
        {
            link.Send(packet);
            lock (timeLock)
                lastActiveTime = DateTime.Now;
        }

        /// <summary>
        /// The background thread is controled by the status register, and the contents of
        /// the buffers and windows. It is in a never ending loop of checking status and 
        /// trying to send data. This thread blocks on the link Send() call.
        /// </summary>
        private void bw_SendData(object s, DoWorkEventArgs e)
        {
            int sleep_millis = 1000;
            Status currentStatus = this.status;
            while (true)
            {
                lock (statusLock)
                    currentStatus = this.status;
                //TODO::if an explicit NOACK is required, do it.
                //if the next SendCacheItem is a stream, preemt the send
                //otherwise let the main switch add data to the packet
                switch (currentStatus)
                { 
                    case Status.connecting:
                        sleep_millis = 1000;//change the sleep time to the ping delay
                        Packet pingPacket = new Packet();
                        pingPacket.Header.LinkStateProp = (byte)PacketHeader.LinkState.CONNECT;
                        pingPacket.Header.SequenceNum = nextSeqNum++;
                        SendPacket(pingPacket);
                        break;

                    case Status.connected:
                        sleep_millis = 0;
                        if (ResendData())//try and resend unacked, expired packets
                            continue;//recycle this case statment from the begining
                        else if (SendCachedData())//try and send new data
                            continue;
                        else if (ackBuffer.Count > 0)
                        {   //if an acknowledgment is waiting, send it
                            Packet ackPacket = new Packet(packetTemplate);
                            AddAcknowledgments(ref ackPacket);
                            SendPacket(ackPacket);
                        }
                        else
                        {   //if the connection has been idle for too long, ping the target
                            TimeSpan idleTime = DateTime.Now - lastActiveTime;
                            if (idleTime.Seconds > 3)
                            {
                                Packet idlePacket = new Packet(packetTemplate);
                                idlePacket.Header.LinkStateProp = (byte)PacketHeader.LinkState.PING;
                                AddAcknowledgments(ref idlePacket);
                                SendPacket(idlePacket);//resets lastActiveTime automatically
                            }
                        }
                        break;

                    case Status.disconnected:
                        sleep_millis = 1000;
                        //TODO::is it possible to cancel an active write to the serial 
                        //      port without screwing up the serial port? we want to 
                        //      just release it to other sessions, not close it. how?
                        ackBuffer.Clear();
                        retransmitWindow.Clear();
                        sendCache.Clear();
                        break;

                    default:
                        break;
                }
                Thread.Sleep(sleep_millis);
            }
        }
        #endregion
    }//class Session
}//namespace SessionManagement
