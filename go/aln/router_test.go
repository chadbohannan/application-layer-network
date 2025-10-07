package aln

import (
	"fmt"
	"testing"
	"time"
)

func TestRoute0Hop(t *testing.T) {
	packetRecieved := false
	rtr := NewRouter("1")
	rtr.RegisterService("ping", func(*Packet) {
		packetRecieved = true
	})

	pkt := NewPacket()
	pkt.DestAddr = "1"
	pkt.Service = "ping"
	rtr.Send(pkt) // blocking call to local handler
	if !packetRecieved {
		t.Fatal("packet not recieved")
	}
}

func TestRoute1Hop(t *testing.T) {
	packetRecieved := false
	rtr1 := NewRouter("1")
	rtr2 := NewRouter("2")

	rtr1.RegisterService("ping", func(*Packet) { packetRecieved = true })

	localChannel := NewLocalChannel()
	rtr1.AddChannel(localChannel)
	rtr2.AddChannel(localChannel.FlippedChannel())

	time.Sleep(time.Millisecond)

	pkt := NewPacket()
	pkt.DestAddr = "1"
	pkt.Service = "ping"
	if err := rtr2.Send(pkt); err != nil {
		t.Fatal(err)
	}
	time.Sleep(time.Millisecond) // let async operation settle
	if !packetRecieved {
		t.Fatal("packet not recieved")
	}
}

func TestRoute2Hop(t *testing.T) {
	packetRecieved := false
	rtr1 := NewRouter("1")
	rtr1.RegisterService("ping", func(*Packet) {
		packetRecieved = true
	})
	rtr2 := NewRouter("2")
	rtr3 := NewRouter("3")

	channelA := NewLocalChannel()
	rtr1.AddChannel(channelA)
	rtr2.AddChannel(channelA.FlippedChannel())

	time.Sleep(10 * time.Millisecond) // allow route and service table to sync

	channelB := NewLocalChannel()
	rtr2.AddChannel(channelB)
	rtr3.AddChannel(channelB.FlippedChannel())

	time.Sleep(10 * time.Millisecond) // allow route and service table to sync

	pkt := NewPacket()
	pkt.Service = "ping"
	if err := rtr3.Send(pkt); err != nil {
		t.Fatal(err)
	}

	time.Sleep(200 * time.Millisecond) // let async operation settle
	if !packetRecieved {
		t.Fatal("packet not recieved")
	}
}

func TestMulticastWithSelf(t *testing.T) {
	receivedPacket1 := false
	rtr1 := NewRouter("1")
	rtr1.RegisterService("ping", func(*Packet) {
		if receivedPacket1 {
			t.Error("packet1 already received")
		}
		receivedPacket1 = true
	})

	receivedPacket2 := false
	rtr2 := NewRouter("2")
	rtr2.RegisterService("ping", func(*Packet) {
		if receivedPacket2 {
			t.Error("packet2 already received")
		}
		receivedPacket2 = true
	})

	receivedPacket3 := false
	rtr3 := NewRouter("3")
	rtr3.RegisterService("ping", func(*Packet) {
		if receivedPacket3 {
			t.Error("packet3 already received")
		}
		receivedPacket3 = true
	})

	channelA := NewLocalChannel()
	rtr1.AddChannel(channelA)
	rtr2.AddChannel(channelA.FlippedChannel())

	time.Sleep(100 * time.Millisecond) // allow route and service table to sync

	channelB := NewLocalChannel()
	rtr2.AddChannel(channelB)
	rtr3.AddChannel(channelB.FlippedChannel())

	time.Sleep(200 * time.Millisecond) // allow route and service table to sync

	pkt := NewPacket()
	pkt.Service = "ping"
	if err := rtr1.Send(pkt); err != nil {
		t.Fatal(err)
	}

	time.Sleep(200 * time.Millisecond) // allow route and service table to sync

	if !receivedPacket1 {
		t.Fatal("!receivedPacket1")
	}
	if !receivedPacket2 {
		t.Fatal("!receivedPacket2")
	}
	if !receivedPacket3 {
		t.Fatal("!receivedPacket3")
	}
}

func TestRoute1LimitedHop(t *testing.T) {
	packetRecieved := false
	rtr1 := NewRouter("1")
	rtr2 := NewRouter("2")

	rtr1.RegisterService("ping", func(*Packet) { packetRecieved = true })

	localChannel := NewLocalChannel()

	count := 0
	canSend := func(resourceID string, packet *Packet) (bool, error) {
		count += 1
		return true, nil
	}

	limitedChannel := NewLimitedChannel(localChannel, string(rtr1.Address()), canSend)

	rtr1.AddChannel(limitedChannel)
	rtr2.AddChannel(localChannel.FlippedChannel())

	time.Sleep(time.Millisecond)

	pkt := NewPacket()
	pkt.DestAddr = "1"
	pkt.Service = "ping"
	if err := rtr2.Send(pkt); err != nil {
		t.Fatal(err)
	}
	time.Sleep(time.Millisecond) // let async operation settle
	if !packetRecieved {
		t.Fatal("packet not recieved")
	}
	if count != 5 { // routing + data packets
		t.Fatalf("expected count == 5, not %d", count)
	}
}

func TestServiceCapacityChangedHandler(t *testing.T) {
	rtr1 := NewRouter("1")
	rtr2 := NewRouter("2")

	// Track service capacity changes
	serviceChanges := make([]string, 0)
	changeChannel := make(chan bool, 10)

	rtr2.SetOnServiceCapacityChangedHandler(func(service string, capacity uint16, address AddressType) {
		serviceChanges = append(serviceChanges, fmt.Sprintf("%s:%d@%s", service, capacity, address))
		changeChannel <- true
	})

	// Connect routers
	localChannel := NewLocalChannel()
	rtr1.AddChannel(localChannel)
	rtr2.AddChannel(localChannel.FlippedChannel())

	time.Sleep(10 * time.Millisecond) // allow initial routing to sync

	// Register a service on rtr1 - should trigger notification on rtr2
	rtr1.RegisterService("test-service", func(*Packet) {})

	// Wait for service discovery
	select {
	case <-changeChannel:
		// Success
	case <-time.After(100 * time.Millisecond):
		t.Fatal("Service registration not detected")
	}

	if len(serviceChanges) != 1 {
		t.Fatalf("Expected 1 service change, got %d: %v", len(serviceChanges), serviceChanges)
	}

	expected := "test-service:1@1"
	if serviceChanges[0] != expected {
		t.Fatalf("Expected '%s', got '%s'", expected, serviceChanges[0])
	}

	// Unregister the service - should trigger removal notification
	rtr1.UnregisterService("test-service")

	// Wait for service removal
	select {
	case <-changeChannel:
		// Success
	case <-time.After(100 * time.Millisecond):
		t.Fatal("Service unregistration not detected")
	}

	if len(serviceChanges) != 2 {
		t.Fatalf("Expected 2 service changes, got %d: %v", len(serviceChanges), serviceChanges)
	}

	expected = "test-service:0@1"
	if serviceChanges[1] != expected {
		t.Fatalf("Expected '%s', got '%s'", expected, serviceChanges[1])
	}
}

func TestServiceCapacityChangedHandlerMultipleServices(t *testing.T) {
	rtr1 := NewRouter("1")
	rtr2 := NewRouter("2")
	rtr3 := NewRouter("3")

	// Track service capacity changes on rtr3
	serviceChanges := make([]string, 0)
	changeChannel := make(chan bool, 10)

	rtr3.SetOnServiceCapacityChangedHandler(func(service string, capacity uint16, address AddressType) {
		serviceChanges = append(serviceChanges, fmt.Sprintf("%s:%d@%s", service, capacity, address))
		changeChannel <- true
	})

	// Create network: rtr1 <-> rtr2 <-> rtr3
	channelA := NewLocalChannel()
	rtr1.AddChannel(channelA)
	rtr2.AddChannel(channelA.FlippedChannel())

	time.Sleep(10 * time.Millisecond)

	channelB := NewLocalChannel()
	rtr2.AddChannel(channelB)
	rtr3.AddChannel(channelB.FlippedChannel())

	time.Sleep(50 * time.Millisecond) // allow routing to sync

	// Register services on different routers
	rtr1.RegisterService("service-a", func(*Packet) {})
	time.Sleep(10 * time.Millisecond) // allow service-a to propagate
	rtr2.RegisterService("service-b", func(*Packet) {})

	// Wait for both service discoveries
	timeout := time.After(200 * time.Millisecond)
	receivedCount := 0

	for receivedCount < 2 {
		select {
		case <-changeChannel:
			receivedCount++
		case <-timeout:
			t.Fatalf("Only received %d/2 service notifications: %v", receivedCount, serviceChanges)
		}
	}

	if len(serviceChanges) != 2 {
		t.Fatalf("Expected 2 service changes, got %d: %v", len(serviceChanges), serviceChanges)
	}

	// Verify both services were discovered (order may vary)
	found := make(map[string]bool)
	for _, change := range serviceChanges {
		found[change] = true
	}

	if !found["service-a:1@1"] {
		t.Fatal("service-a not discovered")
	}
	if !found["service-b:1@2"] {
		t.Fatal("service-b not discovered")
	}
}
