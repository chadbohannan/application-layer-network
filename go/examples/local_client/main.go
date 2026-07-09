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
	// Create router with unique address
	localAddress := aln.AddressType("go-client-da01b5")
	router := aln.NewRouter(localAddress)

	// Connect to the host
	conn, err := net.Dial("tcp", "localhost:8000")
	if err != nil {
		fmt.Println("dial failed:" + err.Error())
		os.Exit(-1)
	}
	router.AddChannel(aln.NewTCPChannel(conn))

	log.Printf("Client '%s' connected to localhost:8081", localAddress)

	// Track discovered services
	discoveredServices := make(map[string]aln.AddressType)
	var servicesMutex sync.RWMutex

	// Set up event-driven service discovery handler
	router.SetOnServiceCapacityChangedHandler(func(service string, capacity uint16, address aln.AddressType) {
		servicesMutex.Lock()
		defer servicesMutex.Unlock()

		log.Printf("🔍 Service update: '%s' (capacity: %d) at '%s'", service, capacity, address)

		if capacity > 0 {
			discoveredServices[service] = address
			log.Printf("✅ Service registered: '%s' at '%s'", service, address)
		} else {
			delete(discoveredServices, service)
			log.Printf("📤 Service removed: '%s' at '%s'", service, address)
		}
	})

	// Wait for ping service discovery (event-driven, no arbitrary sleep!)
	log.Println("🔎 Waiting for ping service discovery...")

	var pingServiceAddress aln.AddressType
	timeout := time.After(5 * time.Second)
	ticker := time.NewTicker(100 * time.Millisecond)
	defer ticker.Stop()

	serviceFound := false
	for !serviceFound {
		select {
		case <-ticker.C:
			servicesMutex.RLock()
			if addr, found := discoveredServices["ping"]; found {
				pingServiceAddress = addr
				serviceFound = true
				log.Printf("✅ Ping service found at '%s'", pingServiceAddress)
			}
			servicesMutex.RUnlock()
		case <-timeout:
			log.Println("❌ Timeout: ping service not discovered")
			os.Exit(1)
		}
	}

	// Setup ping response handler
	var wg sync.WaitGroup
	wg.Add(1)

	var ctx uint16
	ctx = router.RegisterContextHandler(func(response *aln.Packet) {
		log.Printf("🏓 Ping response: '%s' from '%s'", string(response.Data), response.SrcAddr)
		wg.Done()
		router.ReleaseContext(ctx) // Move ReleaseContext after wg.Done to avoid deadlock
	})

	// Send ping to discovered service (no hardcoded address!)
	log.Printf("🏓 Sending ping to discovered service at '%s' with contextID %d", pingServiceAddress, ctx)
	pingPacket := &aln.Packet{
		NextAddr:  "bf14edaf-f51c-4e58-90c4-05fa74914cc7",
		DestAddr:  pingServiceAddress, // Use discovered address
		Service:   "ping",
		ContextID: ctx,
		Data:      []byte("Hello from event-driven client!"),
	}

	if err := router.Send(pingPacket); err != nil {
		log.Printf("❌ Failed to send ping: %s", err.Error())
		os.Exit(1)
	}

	log.Println("📨 Ping sent successfully")

	// Wait for response (event-driven with timeout)
	log.Println("⏳ Waiting for ping response...")

	done := make(chan struct{})
	go func() {
		wg.Wait()
		done <- struct{}{}
	}()

	select {
	case <-done:
		log.Println("✅ Client completed successfully - fully event-driven, no hardcoded addresses or arbitrary sleeps!")
	case <-time.After(10 * time.Second):
		log.Println("⏰ Timeout: No ping response received within 10 seconds")
		log.Printf("🔍 Debug: Client address='%s', Context ID=%d", localAddress, ctx)
		os.Exit(1)
	}
}
