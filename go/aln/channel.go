package aln

// Channel sends one packet at a time by some mechanism
type Channel interface {
	Send(*Packet) error
	Receive(PacketCallback, OnCloseCallback)
	Close()
}

type ChannelHost interface {
	OnAccept(func(Channel))
}

// LocalChannel wraps a pair of chan primatives; intended for development and testing
type LocalChannel struct {
	reciever PacketCallback
	outbound chan *Packet
	inbound  chan *Packet
	close    chan bool
}

// NewLocalChannel allocates the chans of a new LocalChannel
func NewLocalChannel() *LocalChannel {
	return &LocalChannel{
		outbound: make(chan *Packet, 2),
		inbound:  make(chan *Packet, 2),
		close:    make(chan bool),
	}
}

// FlippedChannel returns the channel from the reverse persepective
func (lc *LocalChannel) FlippedChannel() *LocalChannel {
	return &LocalChannel{
		outbound: lc.inbound,
		inbound:  lc.outbound,
	}
}

// Send transmits immediately
func (lc *LocalChannel) Send(packet *Packet) error {
	packet.SetControlFlags()
	lc.outbound <- packet
	return nil
}

// Receive starts a go routine to call onPacket
func (lc *LocalChannel) Receive(onPacket PacketCallback, onClose OnCloseCallback) {
	go func() {
		for {
			select {
			case packet := <-lc.inbound:
				onPacket(packet)
			case <-lc.close:
				if onClose != nil {
					onClose(lc)
				}
				return
			}
		}
	}()
}

// Close releases the Recieve go routine; no other cleanup required
func (lc *LocalChannel) Close() {
	lc.close <- true
}
