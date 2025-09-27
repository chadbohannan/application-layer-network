package main

import (
	"fmt"
	"log"
	"os"
	"os/signal"

	"github.com/chadbohannan/application-layer-network/go/aln"
)

const host = "localhost"
const port = 8081

func main() {
	fmt.Printf("listening to %s:%d\n", host, port)
	alnHostAddress := aln.AddressType("go-host-3f99ea")
	router := aln.NewRouter(alnHostAddress)
	tcpHost := aln.NewTcpChannelHost(host, port)
	go tcpHost.Listen(func(newChannel aln.Channel) {
		router.AddChannel(newChannel)
	})

	// create the ping service to send packets back where they came from
	router.RegisterService("ping", func(packet *aln.Packet) {
		log.Printf("ping from '%s'", packet.SrcAddr)
		response := &aln.Packet{
			SrcAddr:   alnHostAddress,  // Add source address for proper routing
			DestAddr:  packet.SrcAddr,
			ContextID: packet.ContextID,
			Data:      []byte("pong!"),
		}
		if err := router.Send(response); err != nil {
			log.Printf("failed to send pong response: %v", err)
		}
	})

	// wait for ^C
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)
	<-c
}
