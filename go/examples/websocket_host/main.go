package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"

	"github.com/chadbohannan/application-layer-network/go/aln"
	"github.com/gorilla/websocket"
)

const HTTP_PORT = 8080
const TCP_PORT = 8081

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true // Allow all origins for development
	},
}

func main() {
	fmt.Printf("Starting ALN WebSocket host on port %d\n", HTTP_PORT)
	alnHostAddress := aln.AddressType("go-ws-host-" + fmt.Sprintf("%06x", os.Getpid())[:6])
	router := aln.NewRouter(alnHostAddress)

	// Create the ping service to send packets back where they came from
	router.RegisterService("ping", func(packet *aln.Packet) {
		log.Printf("ping from '%s' with data: %s", packet.SrcAddr, string(packet.Data))
		response := &aln.Packet{
			SrcAddr:   alnHostAddress,
			DestAddr:  packet.SrcAddr,
			ContextID: packet.ContextID,
			Data:      []byte("pong from " + string(alnHostAddress) + "!"),
		}
		if err := router.Send(response); err != nil {
			log.Printf("failed to send pong response: %v", err)
		}
	})

	// WebSocket handler
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("WebSocket upgrade error: %v", err)
			return
		}

		log.Printf("New WebSocket connection from %s", conn.RemoteAddr())

		// Create ALN channel and add to router
		wsChannel := aln.NewWebSocketChannel(conn)
		router.AddChannel(wsChannel)

		// Start receiving packets
		wsChannel.Receive(func(p *aln.Packet) bool {
			// Packets are handled by the router via onPacket callback
			return true
		})

		log.Printf("WebSocket connection closed: %s", conn.RemoteAddr())
	})

	// Start HTTP server in goroutine
	go func() {
		addr := fmt.Sprintf(":%d", HTTP_PORT)
		log.Printf("Listening for WebSocket connections on %s", addr)
		if err := http.ListenAndServe(addr, nil); err != nil {
			log.Fatal(err)
		}
	}()

	// Also start a TCP listener for compatibility with other examples
	tcpHost := aln.NewTcpChannelHost("localhost", TCP_PORT)
	go tcpHost.Listen(func(newChannel aln.Channel) {
		log.Printf("New TCP connection")
		router.AddChannel(newChannel)
	})
	log.Printf("Also listening for TCP connections on localhost:8081")

	// Wait for ^C
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt)
	fmt.Println("\nPress Ctrl+C to exit")
	<-c
	fmt.Println("\nShutting down...")
}
