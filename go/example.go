package main

import (
	"fmt"
	"net"
	"os"
	"time"

	"github.com/chadbohannan/expanding-link-protocol/go/elp"
)

func onError(errMsg string) {
	fmt.Println(errMsg)
}

func main() {
	// create first router
	r1Address := elp.AddressType(1)
	r2Address := elp.AddressType(2)
	pingServiceID := uint16(3)

	// setup the first node to be a passive host
	r1 := elp.NewRouter(r1Address, onError)
	tcpHost := elp.NewTcpChannelHost("localhost", 8000)
	go tcpHost.Listen(func(newChannel elp.Channel) {
		fmt.Println("on new channel")
		r1.AddChannel(newChannel)
	})

	// create the ping service
	r1.RegisterService(pingServiceID, func(packet *elp.Packet) {
		fmt.Printf("...")
		r1.Send(&elp.Packet{
			DestAddr:  packet.SrcAddr,
			ContextID: packet.ContextID,
			Data:      []byte("pong"),
		})
	})
	time.Sleep(time.Second)

	// setup the second node to connect to the first using TCP
	r2 := elp.NewRouter(r2Address, onError)
	conn, err := net.Dial("tcp", "localhost:8000")
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}

	r2.AddChannel(elp.NewTCPChannel(conn))

	// give the router time to receive the routing table
	time.Sleep(time.Millisecond)

	// Setup our ping reply handler
	ctx := r2.RegisterContextHandler(func(response *elp.Packet) {
		fmt.Printf("%s", string(response.Data))
	})
	defer r2.ReleaseContext(ctx)

	fmt.Printf("Ping:")
	r2.Send(&elp.Packet{
		DestAddr:  1,
		ServiceID: pingServiceID,
		ContextID: ctx,
	})

	time.Sleep(1 * time.Second)

}
