package elp

import (
	"fmt"
	"net"
	"os"
	"sync"
)

// implements ChannelHost
type TcpChannelHost struct {
	name string
	port int
}

func NewTcpChannelHost(name string, port int) *TcpChannelHost {
	return &TcpChannelHost{
		name: name,
		port: port,
	}
}

func (host *TcpChannelHost) Listen(onConnect func(Channel)) {
	// Listen for incoming connections.
	bindAddress := fmt.Sprintf("%s:%d", host.name, host.port)
	l, err := net.Listen("tcp", bindAddress)
	if err != nil {
		fmt.Println("TCPHost:Listen err:", err.Error())
		os.Exit(1)
	}
	defer l.Close()

	fmt.Println("ELP:TcpChannelHost Listening on " + bindAddress)
	for {
		// Listen for an incoming connection.
		conn, err := l.Accept()
		if err != nil {
			fmt.Println("TCPHost:Accept err: ", err.Error())
			os.Exit(1)
		}
		go onConnect(NewTCPChannel(conn))
	}
}

type TCPChannel struct {
	mutex sync.Mutex
	conn  net.Conn
}

func NewTCPChannel(conn net.Conn) *TCPChannel {
	return &TCPChannel{
		conn: conn,
	}
}

func (ch *TCPChannel) Send(p *Packet) error {
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
		return fmt.Errorf("TCPChannel conn.Write sent %d/%d bytes", n, len(pktBytes))
	}
	return nil
}

func (ch *TCPChannel) Receive(onPacket PacketCallback) {
	parser := NewParser(onPacket)
	byteBuff := []byte{0}
	for {
		n, err := ch.conn.Read(byteBuff)
		if n < 1 || err != nil {
			break
		}
		parser.IngestStream(byteBuff)
	}
	ch.Close()
}

// Close .
func (ch *TCPChannel) Close() {
	ch.conn.Close()
}

// OpenTCPChannel attempts to create an ELP wrapped socket connection to a remote host
func OpenTCPChannel(hostname string, port int) (Channel, error) {
	conn, err := net.Dial("tcp", fmt.Sprintf("%s:%d", hostname, port))
	if err != nil {
		return nil, err
	}
	return NewTCPChannel(conn), nil
}
