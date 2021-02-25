package elp

import (
	"fmt"
	"testing"
	"time"
)

func TestRoute0Hop(t *testing.T) {
	packetRecieved := false
	rtr := NewRouter(1, func(err string) {
		fmt.Println(err)
	})
	rtr.RegisterService(0x0001, func(*Packet) {
		packetRecieved = true
	})

	pkt := NewPacket()
	pkt.DestAddr = 0x0001
	pkt.ServiceID = 0x0001
	rtr.Send(pkt) // blocking call to local handler
	if !packetRecieved {
		t.Fatal("packet not recieved")
	}
}

func TestRoute1Hop(t *testing.T) {
	packetRecieved := false
	rtr1 := NewRouter(1, func(err string) { fmt.Println("router 1 err:" + err) })
	rtr2 := NewRouter(2, func(err string) { fmt.Println("router 2 err:" + err) })

	rtr1.RegisterService(0x0001, func(*Packet) { packetRecieved = true })

	localChannel := NewLocalChannel()
	rtr1.AddChannel(localChannel)
	rtr2.AddChannel(localChannel.FlippedChannel())

	time.Sleep(time.Millisecond)

	pkt := NewPacket()
	pkt.DestAddr = 0x0001
	pkt.ServiceID = 0x0001
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
	rtr1 := NewRouter(1, func(err string) {
		fmt.Println("router 1 err:" + err)
	})
	rtr1.RegisterService(0x0001, func(*Packet) {
		packetRecieved = true
	})
	rtr2 := NewRouter(2, func(err string) {
		fmt.Println("router 2 err:" + err)
	})
	rtr3 := NewRouter(3, func(err string) {
		fmt.Println("router 2 err:" + err)
	})

	channelA := NewLocalChannel()
	rtr1.AddChannel(channelA)
	rtr2.AddChannel(channelA.FlippedChannel())

	channelB := NewLocalChannel()
	rtr2.AddChannel(channelB)
	rtr3.AddChannel(channelB.FlippedChannel())

	time.Sleep(time.Millisecond)

	pkt := NewPacket()
	pkt.DestAddr = 0x0001
	pkt.ServiceID = 0x0001
	if err := rtr3.Send(pkt); err != nil {
		t.Fatal(err)
	}

	time.Sleep(time.Millisecond) // let async operation settle
	if !packetRecieved {
		t.Fatal("packet not recieved")
	}
}
