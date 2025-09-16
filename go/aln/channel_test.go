package aln

import (
	"crypto/tls"
	"log"
	"net"
	"net/http"
	"net/http/httptest"
	"sync"
	"testing"
	"time"

	"github.com/gorilla/websocket"
)

func TestSSLChannelOnClose(t *testing.T) {
	// Create a pair of connected in-memory pipes
	clientConn, _ := net.Pipe()

	// Create a dummy tls.Conn for NewSSLChannel
	// The actual TLS handshake is not relevant for testing OnClose handler invocation
	sslClientConn := tls.Client(clientConn, &tls.Config{InsecureSkipVerify: true})

	// Create an SSLChannel with the client side
	channel := NewSSLChannel(sslClientConn)

	var wg sync.WaitGroup
	wg.Add(1)

	channel.OnClose(func(c Channel) {
		wg.Done()
	})

	// Call Close directly to trigger the OnClose handler
	channel.Close()

	done := make(chan struct{})
	go func() {
		wg.Wait()
		close(done)
	}()

	select {
	case <-done:
		// OnClose was called, test passed
	case <-time.After(5 * time.Second):
		t.Fatal("OnClose was not called within 5 seconds")
	}
}

func TestWebSocketChannelOnClose(t *testing.T) {
	log.Printf("Starting TestWebSocketChannelOnClose")
	// Create a test WebSocket server
	s := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		upgrader := websocket.Upgrader{}
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			t.Fatalf("Failed to upgrade websocket: %v", err)
		}
		defer conn.Close()
		// Keep the server connection open until client closes
		for {
			_, _, err := conn.ReadMessage()
			if err != nil {
				return
			}
		}
	}))
	defer s.Close()

	// Connect a WebSocket client to the test server
	wsURL := "ws" + s.URL[len("http"):]
	clientConn, _, err := websocket.DefaultDialer.Dial(wsURL, nil)
	if err != nil {
		t.Fatalf("Failed to connect to websocket server: %v", err)
	}

	rtr := NewRouter("testRouter") // Create a new router for this test
	channel := NewWebSocketChannel(clientConn)
	rtr.AddChannel(channel)
	defer rtr.RemoveChannel(channel) // Ensure channel is removed from router

	var wg sync.WaitGroup
	wg.Add(1)

	channel.OnClose(func(c Channel) {
		wg.Done()
	})

	// Start the Receive method in a goroutine
	go channel.Receive(func(p *Packet) bool { return true })

	// Close the client connection to simulate a disconnect
	clientConn.Close()
	channel.Close()

	done := make(chan struct{})
	go func() {
		wg.Wait()
		close(done)
	}()

	select {
	case <-done:
		// OnClose was called, test passed
	case <-time.After(5 * time.Second):
		t.Fatal("OnClose was not called within 5 seconds")
	}
}
