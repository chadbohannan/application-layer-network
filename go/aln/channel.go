package aln

import (
	"fmt"
	"sync"
)

// Channel sends one packet at a time by some mechanism
type Channel interface {
	Send(*Packet) error
	Receive(PacketCallback)
	Close()
	OnClose(...OnCloseCallback)
}

type ChannelHost interface {
	OnAccept(func(Channel))
}

// LocalChannel wraps a pair of chan primatives; intended for development and testing
type LocalChannel struct {
	outbound    chan *Packet
	inbound     chan *Packet
	close       chan bool
	onCloseOnce sync.Once
	onClose     []OnCloseCallback
}

// NewLocalChannel allocates the chans of a new LocalChannel
func NewLocalChannel() *LocalChannel {
	return &LocalChannel{
		// buffer of less than 5 can hang TestMulticastWithSelf
		outbound: make(chan *Packet, 6),
		inbound:  make(chan *Packet, 6),
		close:    make(chan bool),
		onClose:  make([]OnCloseCallback, 0),
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
	select {
	case <-lc.close:
		return fmt.Errorf("LocalChannel is closed, cannot send packet")
	case lc.outbound <- packet:
		return nil
	}
}

// Receive starts a go routine to call onPacket
func (lc *LocalChannel) Receive(onPacket PacketCallback) {
	go func() {
		cont := true
		for cont {
			select {
			case packet := <-lc.inbound:
				cont = onPacket(packet)
			case <-lc.close:
				return
			}
		}
	}()
}

// Close releases the Recieve go routine; no other cleanup required
func (lc *LocalChannel) Close() {
	lc.close <- true
	lc.onCloseOnce.Do(func() {
		for _, handler := range lc.onClose {
			handler(lc)
		}
	})
}

// OnClose registers handlers to be notified of channel teardown
func (lc *LocalChannel) OnClose(onClose ...OnCloseCallback) {
	lc.onClose = append(lc.onClose, onClose...)
}
