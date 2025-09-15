package aln

import "fmt"

// LimitedChannel wraps another Channel and adds a CanSend callback to enable
// a server implmentation to control the sending of packets.
// This allows injecting business logic into enabling the sending of packets.
type LimitedChannel struct {
	wrapped         Channel
	router          *Router
	CanSend         func(*Router, *Packet) (bool, error)
	onCloseHandlers []OnCloseCallback
}

// NewLimitedChannel wraps another channel with a new LimitedChannel
func NewLimitedChannel(ch Channel, router *Router, canSend func(*Router, *Packet) (bool, error)) *LimitedChannel {
	return &LimitedChannel{
		wrapped:         ch,
		router:          router,
		CanSend:         canSend,
		onCloseHandlers: make([]OnCloseCallback, 0),
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
func (lc *LimitedChannel) Receive(onPacket PacketCallback) {
	lc.wrapped.Receive(onPacket)
}

// Close releases the Recieve go routine; no other cleanup required
func (lc *LimitedChannel) Close() {
	lc.wrapped.Close()
	for _, f := range lc.onCloseHandlers {
		f(lc)
	}
}

// OnClose registers handlers to be notified of channel teardown
func (lc *LimitedChannel) OnClose(onClose ...OnCloseCallback) {
	lc.onCloseHandlers = append(lc.onCloseHandlers, onClose...)
}
