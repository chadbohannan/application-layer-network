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
	cloudHost = "localhost:8000"
	cloudNode = "bf14edaf-f51c-4e58-90c4-05fa74914cc7"
)

func main() {
	// create first router
	localAddress := aln.AddressType("go-client-da01b5")
	router := aln.NewRouter(localAddress)

	// connect to the web
	conn, err := net.Dial("tcp", cloudHost)
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}
	// Wrap TCP channel with ViaChannel for static routing through cloud node
	tcpCh := aln.NewTCPChannel(conn)
	cloudCh := aln.NewViaChannel(tcpCh, cloudNode)
	router.AddChannel(cloudCh)

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
