package elp

// Channel sends one packet at a time by some mechanism
type Channel interface {
	Send(*Packet) error
	Receive(func(*Packet))
	Close()
}

// TCPChannel is a serialized, non-shared channel, but multiple nodes
//  may be accessible through it.
// type TCPChannel struct {
// 	onPacket     func(*Packet)
// 	onDisconnect func(Channel)
// }

// func NewTCPChannel() {
//   TODO
// }

// LocalChannel wraps a pair of chan primatives; intended for development and test
type LocalChannel struct {
	outbound chan *Packet
	inbound  chan *Packet
	close    chan bool
}

// NewLocalChannel allocates the chans of a new LocalChannel
func NewLocalChannel() *LocalChannel {
	return &LocalChannel{
		outbound: make(chan *Packet),
		inbound:  make(chan *Packet),
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
	lc.outbound <- packet
	return nil
}

// Receive starts a go routine to call onPacket
func (lc *LocalChannel) Receive(onPacket func(*Packet)) {
	go func() {
		select {
		case packet := <-lc.inbound:
			onPacket(packet)
		case <-lc.close:
			return
		}
	}()
}

// Close releases the Recieve go routine; no other cleanup required
func (lc *LocalChannel) Close() {
	lc.close <- true
}
