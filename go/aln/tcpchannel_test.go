package aln

import (
	"net"
	"sync"
	"testing"
	"time"
)

func TestTcpChannelOnClose(t *testing.T) {
	// Create a pair of connected in-memory pipes
	clientConn, serverConn := net.Pipe()

	// Create a TcpChannel with the client side of the pipe
	channel := NewTCPChannel(clientConn)

	// Start the Receive method in a goroutine to process incoming data and detect connection closure
	go channel.Receive(func(p *Packet) bool { return true }, func(c Channel) {})

	// Use a wait group to synchronize the test with the OnClose callback
	var wg sync.WaitGroup
	wg.Add(1)

	// Configure the OnClose method
	channel.OnClose(func(c Channel) {
		wg.Done()
	})

	// Close the server side of the pipe to simulate an unexpected disconnect
	serverConn.Close()

	// Wait for the OnClose method to be called
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

	// Close the channel to clean up resources
	channel.Close()
}
