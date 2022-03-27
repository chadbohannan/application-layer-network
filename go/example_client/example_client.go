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
	localAddress := aln.AddressType("client")
	pingServiceID := uint16(3)

	// setup the second node to connect to the first using TCP
	router := aln.NewRouter(localAddress)
	conn, err := net.Dial("tcp", "localhost:8181")
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}

	router.AddChannel(aln.NewTCPChannel(conn))

	// Setup our ping reply handler
	var wg sync.WaitGroup
	wg.Add(1)
	ctx := router.RegisterContextHandler(func(response *aln.Packet) {
		fmt.Println(string(response.Data)) // pong!
		time.Sleep((100))                  // let print statement flush
		wg.Done()                          // release the application lock
	})
	defer router.ReleaseContext(ctx)

	fmt.Println("ping") // the journey begins
	if err := router.Send(&aln.Packet{
		ServiceID: pingServiceID,
		ContextID: ctx,
	}); err != nil {
		fmt.Println(err.Error())
	}

	wg.Wait() // wait for pong
}
