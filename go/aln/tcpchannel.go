package aln

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

// Listen for incoming connections. Blocks indefinetly.
func (host *TcpChannelHost) Listen(onConnect func(Channel)) {
	bindAddress := fmt.Sprintf("%s:%d", host.name, host.port)
	l, err := net.Listen("tcp", bindAddress)
	if err != nil {
		fmt.Println("TCPHost:Listen err:", err.Error())
		os.Exit(-1)
	}
	defer l.Close()

	fmt.Println("ALN:TcpChannelHost Listening on " + bindAddress)
	for {
		// Listen for an incoming connection.
		conn, err := l.Accept()
		if err != nil {
			fmt.Println("TCPHost:Accept err: ", err.Error())
			os.Exit(-1)
		}
		go onConnect(NewTCPChannel(conn))
	}
}

// OpenTCPChannel attempts to create an ELP wrapped socket connection to a remote host
func OpenTCPChannel(hostname string, port int) (Channel, error) {
	conn, err := net.Dial("tcp", fmt.Sprintf("%s:%d", hostname, port))
	if err != nil {
		return nil, err
	}
	return NewTCPChannel(conn), nil
}

type TCPChannel struct {
	mutex sync.Mutex
	conn  net.Conn
}

// NewTCPChannel creates a new channel around an existing connection
func NewTCPChannel(conn net.Conn) *TCPChannel {
	return &TCPChannel{
		conn: conn,
	}
}

// Send serializes a packet frame through the socket
func (ch *TCPChannel) Send(p *Packet) error {
	p.SetControlFlags()
	pktBytes, err := p.ToFrameBytes()
	if err != nil {
		return err
	}
	fmt.Printf("send to %s from %s via:%s net:%d ctxID:%d serviceID:%d data:%v\n",
		p.DestAddr, p.SrcAddr, p.NextAddr, p.NetState, p.ContextID, p.ServiceID, p.Data)

	ch.mutex.Lock()
	defer ch.mutex.Unlock()
	if n, err := ch.conn.Write(pktBytes); err != nil {
		return err
	} else if n != len(pktBytes) {
		return fmt.Errorf("TCPChannel conn.Write sent %d/%d bytes", n, len(pktBytes))
	}
	return nil
}

// Receive deserializes packets from it's socket
func (ch *TCPChannel) Receive(onPacket PacketCallback, onClose OnCloseCallback) {
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
	if onClose != nil {
		onClose(ch)
	}
}

// Close .
func (ch *TCPChannel) Close() {
	ch.conn.Close()
}
