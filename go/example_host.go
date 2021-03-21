package main

import (
	"os"
	"os/signal"

	"github.com/chadbohannan/application-layer-network/go/aln"
)

func main() {
	alnHostAddress := aln.AddressType(1)
	pingServiceID := uint16(1)
	router := aln.NewRouter(alnHostAddress)
	tcpHost := aln.NewTcpChannelHost("localhost", 8000)
	go tcpHost.Listen(func(newChannel aln.Channel) {
		router.AddChannel(newChannel)
	})

	// create the ping service to send packets back where they came from
	router.RegisterService(pingServiceID, func(packet *aln.Packet) {
		router.Send(&aln.Packet{
			DestAddr:  packet.SrcAddr,
			ContextID: packet.ContextID,
			Data:      []byte("pong!"),
		})
	})

	// wait for ^C
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)
	<-c
}
