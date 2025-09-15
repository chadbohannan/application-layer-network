package aln

import (
	"crypto/tls"
	"fmt"
	"log"
	"os"
	"sync"
)

// implements ChannelHost
type SslChannelHost struct {
	name     string
	port     int
	keyFile  string
	certFile string
}

func NewSslChannelHost(name string, port int, keyFile, certFile string) *SslChannelHost {
	return &SslChannelHost{
		name:     name,
		port:     port,
		keyFile:  keyFile,
		certFile: certFile,
	}
}

// Listen for incoming connections. Blocks indefinetly.
func (host *SslChannelHost) Listen(onConnect func(Channel)) {
	bindAddress := fmt.Sprintf("%s:%d", host.name, host.port)

	cert, err := tls.LoadX509KeyPair(host.certFile, host.keyFile)
	if err != nil {
		log.Fatal(err)
	}
	config := &tls.Config{Certificates: []tls.Certificate{cert}}

	l, err := tls.Listen("tcp", bindAddress, config)
	if err != nil {
		fmt.Println("SslChannelHost:Listen err:", err.Error())
		os.Exit(-1)
	}
	defer l.Close()

	for {
		conn, err := l.Accept() // Listen for an incoming connection.
		if err != nil {
			fmt.Println("SslChannelHost:Accept err: ", err.Error())
			os.Exit(-1)
		}
		go onConnect(NewSSLChannel(conn.(*tls.Conn)))
	}
}

// OpenSSLChannel attempts to create an ELP wrapped socket connection to a remote host
func OpenSSLChannel(hostname string, port int, config *tls.Config) (Channel, error) {
	conn, err := tls.Dial("tcp", fmt.Sprintf("%s:%d", hostname, port), config)
	if err != nil {
		return nil, err
	}
	return NewSSLChannel(conn), nil
}

type SSLChannel struct {
	mutex        sync.Mutex
	conn         *tls.Conn
	onCloseOnce  sync.Once
	onCloseHandler OnCloseCallback
}

// NewSSLChannel creates a new channel around an existing connection
func NewSSLChannel(conn *tls.Conn) *SSLChannel {
	return &SSLChannel{
		conn: conn,
	}
}

// Send serializes a packet frame through the socket
func (ch *SSLChannel) Send(p *Packet) error {
	p.SetControlFlags()
	pktBytes, err := p.ToFrameBytes()
	if err != nil {
		return err
	}

	ch.mutex.Lock()
	defer ch.mutex.Unlock()
	if n, err := ch.conn.Write(pktBytes); err != nil {
		return err
	} else if n != len(pktBytes) {
		return fmt.Errorf("SSLChannel conn.Write sent %d/%d bytes", n, len(pktBytes))
	}
	return nil
}

// Receive deserializes packets from it's socket
func (ch *SSLChannel) Receive(onPacket PacketCallback, onClose OnCloseCallback) {
	parser := NewParser(onPacket)
	byteBuff := []byte{0}
	for {
		n, err := ch.conn.Read(byteBuff)
		if n < 1 || err != nil {
			break
		}
		if !parser.IngestStream(byteBuff) {
			return
		}
	}
	// ch.Close() // Removed redundant call
	log.Printf("SSLChannel.Receive: exiting")
}

// Close .
func (ch *SSLChannel) Close() {
	log.Printf("SSLChannel.Close: called")
	ch.conn.Close()
	ch.onCloseOnce.Do(func() {
		if ch.onCloseHandler != nil {
			ch.onCloseHandler(ch)
		}
	})
}

// OnClose registers handlers to be notified of channel teardown
func (ch *SSLChannel) OnClose(onClose OnCloseCallback) {
	ch.onCloseHandler = onClose
}
