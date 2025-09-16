package aln

import "fmt"

// LimitedChannel wraps another Channel and adds a CanSend callback to enable
// a server implmentation to control the sending of packets.
// This allows injecting business logic into enabling the sending of packets.
type LimitedChannel struct {
	wrapped    Channel
	resourceID string
	CanSend    func(string, *Packet) (bool, error)
}

// NewLimitedChannel wraps another channel with a new LimitedChannel
func NewLimitedChannel(ch Channel, rID string, canSend func(string, *Packet) (bool, error)) *LimitedChannel {
	return &LimitedChannel{
		wrapped:    ch,
		resourceID: rID,
		CanSend:    canSend,
	}
}

// Send transmits immediately
func (lc *LimitedChannel) Send(packet *Packet) error {
	ok, err := lc.CanSend(lc.resourceID, packet)
	if ok {
		lc.wrapped.Send(packet)
	}
	if err != nil {
		fmt.Printf("LimitedChannel CanSend err:%s", err.Error())
	}
	return err
}

// Receive starts a go routine to call onPacket
func (lc *LimitedChannel) Receive(onPacket PacketCallback) {
	lc.wrapped.Receive(onPacket)
}

// Close releases the Recieve go routine; no other cleanup required
func (lc *LimitedChannel) Close() {
	lc.wrapped.Close()
}

// OnClose registers handlers to be notified of channel teardown
func (lc *LimitedChannel) OnClose(onClose ...OnCloseCallback) {
	lc.wrapped.OnClose(onClose...)
}
