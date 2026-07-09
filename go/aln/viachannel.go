package aln

// ViaChannel wraps a Channel to automatically set NextAddr on all outbound packets.
// This is useful for cloud multiplexing scenarios where all packets must route
// through a specific intermediate node (gateway/proxy).
type ViaChannel struct {
	wrapped Channel
	via     AddressType
}

// NewViaChannel creates a new ViaChannel that wraps the given channel
// and sets NextAddr to the specified address on all outbound packets.
func NewViaChannel(ch Channel, via AddressType) *ViaChannel {
	return &ViaChannel{
		wrapped: ch,
		via:     via,
	}
}

// Send transmits a packet with NextAddr automatically set to the via address.
// The original packet is not modified (a copy is made).
func (vc *ViaChannel) Send(packet *Packet) error {
	// Clone the packet to avoid mutating the caller's copy
	p := *packet
	p.NextAddr = vc.via
	return vc.wrapped.Send(&p)
}

// Receive delegates to the wrapped channel
func (vc *ViaChannel) Receive(onPacket PacketCallback) {
	vc.wrapped.Receive(onPacket)
}

// Close delegates to the wrapped channel
func (vc *ViaChannel) Close() {
	vc.wrapped.Close()
}

// OnClose delegates to the wrapped channel
func (vc *ViaChannel) OnClose(callback ...OnCloseCallback) {
	vc.wrapped.OnClose(callback...)
}
