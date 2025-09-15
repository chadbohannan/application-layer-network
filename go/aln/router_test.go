package aln

import (
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

	time.Sleep(500 * time.Millisecond) // let async operation settle
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

	time.Sleep(100 * time.Millisecond) // allow route and service table to sync

	pkt := NewPacket()
	pkt.Service = "ping"
	if err := rtr1.Send(pkt); err != nil {
		t.Fatal(err)
	}

	time.Sleep(100 * time.Millisecond) // allow route and service table to sync

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
	canSend := func(router *Router, packet *Packet) (bool, error) {
		count += 1
		return true, nil
	}

	limitedChannel := NewLimitedChannel(localChannel, rtr1, canSend)

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
