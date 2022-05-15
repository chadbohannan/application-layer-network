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
	r1Address := aln.AddressType(1)
	r2Address := aln.AddressType(2)

	// setup the first node to host a ping service
	r1 := aln.NewRouter(r1Address)
	tcpHost := aln.NewTcpChannelHost("localhost", 8181)
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

	time.Sleep(10 * time.Millisecond) // let the new server open port 8000

	// setup the second node to connect to the first using TCP
	r2 := aln.NewRouter(r2Address)
	conn, err := net.Dial("tcp", "localhost:8181")
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}

	r2.AddChannel(aln.NewTCPChannel(conn))

	time.Sleep(10 * time.Millisecond) // let the routing tables synchronize

	// Setup our ping reply handler
	var wg sync.WaitGroup
	wg.Add(1)
	ctx := r2.RegisterContextHandler(func(response *aln.Packet) {
		fmt.Println(string(response.Data)) // pong!
		wg.Done()
	})
	defer r2.ReleaseContext(ctx)

	fmt.Println("ping") // the journey begins
	r2.Send(&aln.Packet{
		Service:   "ping",
		ContextID: ctx,
	})

	wg.Wait()
}
