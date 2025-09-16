package aln

import (
	"sync"
	"testing"
	"time"
)

func TestLimitedChannelOnClose(t *testing.T) {
	// Create a LocalChannel as the underlying channel to be wrapped
	localChannel := NewLocalChannel()

	// Create a LimitedChannel wrapping the LocalChannel
	canSend := func(resouceID string, packet *Packet) (bool, error) {
		return true, nil
	}
	lc := NewLimitedChannel(localChannel, "", canSend)

	lc.Receive(func(p *Packet) bool { return false })

	// Test OnClose behavior
	var wg sync.WaitGroup
	wg.Add(1)

	limitedChannelClosed := false
	lc.OnClose(func(c Channel) {
		limitedChannelClosed = true
		wg.Done()
	})

	localChannel.Close() // Explicitly close the local channel to trigger its OnClose handlers

	// Wait for both close handlers to be called, with a timeout
	done := make(chan struct{})
	go func() {
		wg.Wait()
		close(done)
	}()

	select {
	case <-done:
		// All handlers completed within the timeout
	case <-time.After(1 * time.Second):
		t.Fatal("TestLimitedChannelOnClose timed out")
	}

	if !limitedChannelClosed {
		t.Error("LimitedChannel OnClose handler was not called")
	}
}
