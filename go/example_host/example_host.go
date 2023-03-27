package main

import (
	"log"
	"os"
	"os/signal"

	"github.com/chadbohannan/application-layer-network/go/aln"
)

func main() {
	alnHostAddress := aln.AddressType("go-host-3f99ea")
	router := aln.NewRouter(alnHostAddress)
	tcpHost := aln.NewTcpChannelHost("localhost", 8081)
	go tcpHost.Listen(func(newChannel aln.Channel) {
		router.AddChannel(newChannel)
	})

	// create the ping service to send packets back where they came from
	router.RegisterService("ping", func(packet *aln.Packet) {
		log.Printf("ping from '%s'", packet.SrcAddr)
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
