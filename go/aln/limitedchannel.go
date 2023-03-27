package aln

import "fmt"

// LimitedChannel wraps a pair of chan primatives; intended for development and testing
type LimitedChannel struct {
	wrapped Channel
	router  *Router
	CanSend func(*Router, *Packet) (bool, error)
}

// NewLimitedChannel wraps another channel with a new LimitedChannel
func NewLimitedChannel(ch Channel, router *Router, canSend func(*Router, *Packet) (bool, error)) *LimitedChannel {
	return &LimitedChannel{
		wrapped: ch,
		router:  router,
		CanSend: canSend,
	}
}

// Send transmits immediately
func (lc *LimitedChannel) Send(packet *Packet) error {
	ok, err := lc.CanSend(lc.router, packet)
	if ok {
		lc.wrapped.Send(packet)
	}
	if err != nil {
		fmt.Printf("LimitedChannel CanSend err:%s", err.Error())
	}
	return err
}

// Receive starts a go routine to call onPacket
func (lc *LimitedChannel) Receive(onPacket PacketCallback, onClose OnCloseCallback) {
	lc.wrapped.Receive(onPacket, func(ch Channel) {
		onClose(lc)
	})
}

// Close releases the Recieve go routine; no other cleanup required
func (lc *LimitedChannel) Close() {
	lc.wrapped.Close()
}
