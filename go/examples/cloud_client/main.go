package main

import (
	"fmt"
	"log"
	"net"
	"os"
	"time"

	"github.com/chadbohannan/application-layer-network/go/aln"
)

const (
	cloudHost = "layer7node.net:8000"
	cloudNode = "5b404c2d-50d1-4b07-94af-1b167c64e4e3"
)

func main() {
	// create first router
	localAddress := aln.AddressType("go-client-da01b5")

	// setup the second node to connect to the first using TCP
	router := aln.NewRouter(localAddress)

	// connect to the web
	conn, err := net.Dial("tcp", "layer7node.net:8000")
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}
	ch := aln.NewTCPChannel(conn)
	packet := aln.NewPacket()
	packet.DestAddr = cloudNode // Resolve router node by sending an empty packet to it before anything else
	ch.Send(packet)
	router.AddChannel(ch)

	time.Sleep(1 * time.Second) // let protocol chatter settle

	// send a log message to any 'log' services that might be connected
	log.Println("generating log packet")
	if err := router.Send(&aln.Packet{
		Service: "log",
		Data:    []byte("cloud client says hello"),
	}); err != nil {
		log.Print(err.Error())
	}

	time.Sleep(100 * time.Millisecond) // hang for a moment before returning
}
