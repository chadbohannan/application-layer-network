package main

import (
	"fmt"
	"log"
	"net"
	"os"
	"sync"
	"time"

	"github.com/chadbohannan/application-layer-network/go/aln"
)

func main() {
	// create first router
	localAddress := aln.AddressType("go-client-da01b5")

	// setup the second node to connect to the first using TCP
	router := aln.NewRouter(localAddress)
	conn, err := net.Dial("tcp", "localhost:8000")
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}
	router.AddChannel(aln.NewTCPChannel(conn))

	// Setup our ping reply handler
	var wg sync.WaitGroup
	wg.Add(1)
	var ctx uint16
	ctx = router.RegisterContextHandler(func(response *aln.Packet) {
		// router-trip journey complete
		log.Printf("'%s' from '%s'", string(response.Data), response.SrcAddr)
		time.Sleep(time.Millisecond) // let print statement flush
		router.ReleaseContext(ctx)   // not expecting any more response packets; release this handler
		wg.Done()                    // release the lock
	})

	time.Sleep(time.Second) // let protocol chatter settle

	// the round-trip journey begins
	log.Println("generating ping packet")
	if err := router.Send(&aln.Packet{
		DestAddr:  "go-host-3f99ea",
		Service:   "ping",
		ContextID: ctx, // we must provide ctx because we expect a response
	}); err != nil {
		log.Print(err.Error())
	}

	wg.Wait() // wait for response to release this lock and allow the program to exit
}
