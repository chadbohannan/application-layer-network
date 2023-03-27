package main

import (
	"fmt"
	"net"
	"os"
	"sync"
	"time"

	"github.com/chadbohannan/application-layer-network/go/aln"
)

func main() {
	// create first router
	r1Address := aln.AddressType("r1")
	r2Address := aln.AddressType("r2")

	// setup the first node to host a ping service
	r1 := aln.NewRouter(r1Address)
	tcpHost := aln.NewTcpChannelHost("localhost", 8081)
	go tcpHost.Listen(func(newChannel aln.Channel) {
		r1.AddChannel(newChannel)
	})

	// create the ping service to send packets back where they came from
	r1.RegisterService("ping", func(packet *aln.Packet) {
		fmt.Printf("...") // this is the mid-point of the round-trip
		r1.Send(&aln.Packet{
			DestAddr:  packet.SrcAddr,
			ContextID: packet.ContextID,
			Data:      []byte("pong!"),
		})
	})

	time.Sleep(10 * time.Millisecond) // let the new server open port 8081

	// connect to the first node using TCP
	conn, err := net.Dial("tcp", "localhost:8081")
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}

	// create the second router and add the new channel to it; TcpChannelHost will handle the other side
	r2 := aln.NewRouter(r2Address)
	r2.AddChannel(aln.NewTCPChannel(conn))

	// both routers are now connected
	time.Sleep(10 * time.Millisecond) // let the route and service synchronization finish

	// Setup our ping reply handler
	var wg sync.WaitGroup
	wg.Add(1)
	var ctx uint16
	ctx = r2.RegisterContextHandler(func(response *aln.Packet) {
		fmt.Println(string(response.Data)) // should say "pong!"
		r2.ReleaseContext(ctx)             // release the memory associated with this packet handler
		wg.Done()                          // the ping response unclocks our waitgroup and the program can exit
	})

	fmt.Println("ping") // the journey begins
	r2.Send(&aln.Packet{
		Service:   "ping",
		ContextID: ctx,
	})

	wg.Wait() // blocks until the ping finishes it's round-trip
}
