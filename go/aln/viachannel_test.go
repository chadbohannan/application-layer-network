package aln

import (
	"testing"
	"time"
)

func TestViaChannel_AutomaticNextAddr(t *testing.T) {
	// Setup local channel pair
	chA := NewLocalChannel()
	chB := chA.FlippedChannel()

	targetAddr := AddressType("cloud-router")
	viaCh := NewViaChannel(chA, targetAddr)

	// Track received packet
	var received *Packet
	chB.Receive(func(p *Packet) bool {
		received = p
		return true
	})

	// Send packet without NextAddr
	original := &Packet{
		Service: "test",
		Data:    []byte("hello"),
	}

	viaCh.Send(original)
	time.Sleep(time.Millisecond) // Let async operation settle

	// Verify NextAddr was set
	if received == nil {
		t.Fatal("Packet was not received")
	}
	if received.NextAddr != targetAddr {
		t.Errorf("Expected NextAddr=%s, got %s", targetAddr, received.NextAddr)
	}

	// Verify original packet unchanged
	if original.NextAddr != "" {
		t.Error("Original packet should not be modified")
	}
}

func TestViaChannel_OverwritesExistingNextAddr(t *testing.T) {
	chA := NewLocalChannel()
	chB := chA.FlippedChannel()

	targetAddr := AddressType("cloud-router")
	viaCh := NewViaChannel(chA, targetAddr)

	var received *Packet
	chB.Receive(func(p *Packet) bool {
		received = p
		return true
	})

	// Send packet with different NextAddr
	viaCh.Send(&Packet{
		NextAddr: AddressType("old-address"),
		Data:     []byte("test"),
	})
	time.Sleep(time.Millisecond) // Let async operation settle

	// Verify NextAddr was overwritten
	if received == nil {
		t.Fatal("Packet was not received")
	}
	if received.NextAddr != targetAddr {
		t.Errorf("NextAddr should be overwritten to %s, got %s", targetAddr, received.NextAddr)
	}
}

func TestViaChannel_WithProtocolPackets(t *testing.T) {
	chA := NewLocalChannel()
	chB := chA.FlippedChannel()

	viaAddr := AddressType("gateway")
	viaCh := NewViaChannel(chA, viaAddr)

	var received *Packet
	chB.Receive(func(p *Packet) bool {
		received = p
		return true
	})

	// Send a route advertisement packet
	viaCh.Send(&Packet{
		NetState: NET_ROUTE,
		SrcAddr:  AddressType("node-a"),
		Data:     []byte("route-data"),
	})
	time.Sleep(time.Millisecond) // Let async operation settle

	// Verify NextAddr was set on protocol packet
	if received == nil {
		t.Fatal("Packet was not received")
	}
	if received.NextAddr != viaAddr {
		t.Errorf("Protocol packet NextAddr=%s, expected %s", received.NextAddr, viaAddr)
	}

	if received.NetState != NET_ROUTE {
		t.Error("NetState should be preserved")
	}
}

func TestViaChannel_DelegatesClose(t *testing.T) {
	chA := NewLocalChannel()
	viaCh := NewViaChannel(chA, AddressType("gateway"))

	// Start Receive so Close can complete
	chA.Receive(func(p *Packet) bool { return true })

	closeCalled := false
	chA.OnClose(func(c Channel) {
		closeCalled = true
	})

	viaCh.Close()
	time.Sleep(time.Millisecond) // Let async operation settle

	if !closeCalled {
		t.Error("Close should be delegated to wrapped channel")
	}
}

func TestViaChannel_DelegatesOnClose(t *testing.T) {
	chA := NewLocalChannel()
	viaCh := NewViaChannel(chA, AddressType("gateway"))

	// Start Receive so Close can complete
	chA.Receive(func(p *Packet) bool { return true })

	callbackCalled := false
	viaCh.OnClose(func(c Channel) {
		callbackCalled = true
	})

	chA.Close()
	time.Sleep(time.Millisecond) // Let async operation settle

	if !callbackCalled {
		t.Error("OnClose callback should be registered with wrapped channel")
	}
}

func TestViaChannel_CanWrapAnyChannel(t *testing.T) {
	// Test that ViaChannel can wrap different channel types
	localCh := NewLocalChannel()
	viaLocal := NewViaChannel(localCh, AddressType("via-local"))
	if viaLocal == nil {
		t.Error("Should be able to wrap LocalChannel")
	}

	// Can also wrap another ViaChannel (stacking)
	viaVia := NewViaChannel(viaLocal, AddressType("via-via"))
	if viaVia == nil {
		t.Error("Should be able to wrap ViaChannel")
	}

	// Send through double-wrapped channel
	receiver := localCh.FlippedChannel()
	var received *Packet
	receiver.Receive(func(p *Packet) bool {
		received = p
		return true
	})

	viaVia.Send(&Packet{Data: []byte("test")})
	time.Sleep(time.Millisecond) // Let async operation settle

	// Inner wrapper's address should be used
	if received == nil {
		t.Fatal("Packet was not received")
	}
	if received.NextAddr != AddressType("via-local") {
		t.Errorf("Expected via-local, got %s", received.NextAddr)
	}
}
