package aln

import (
	"fmt"
	"log"
	"sync"

	"github.com/gorilla/websocket"
)

type WebSocketChannel struct {
	mutex sync.Mutex
	conn  *websocket.Conn
}

// NewWebSocketChannel creates a new channel around an existing connection
func NewWebSocketChannel(conn *websocket.Conn) *WebSocketChannel {
	return &WebSocketChannel{
		conn: conn,
	}
}

// Send serializes a packet frame through the socket
func (ch *WebSocketChannel) Send(p *Packet) error {
	fmt.Printf("send to %s from %s via:%s net:%d ctxID:%d serviceID:%d data:%v\n",
		p.DestAddr, p.SrcAddr, p.NextAddr, p.NetState, p.ContextID, p.ServiceID, p.Data)

	ch.mutex.Lock()
	defer ch.mutex.Unlock()
	if _, err := ch.conn.WriteJSON(p); err != nil {
		return err
	}
	return nil
}

// Receive deserializes packets from it's socket
func (ch *WebSocketChannel) Receive(onPacket PacketCallback, onClose OnCloseCallback) {
	for { // ever and ever
		packet := &Packet{}
		if err := ch.conn.ReadJSON(packet); err == nil {
			onPacket(packet)
		} else {
			log.Printf("client read err: %s\nclosing socket: %s", err.Error(), ch.conn.RemoteAddr().String())
			break // jk; not forever
		}
	}
	onClose()
}

// Close .
func (ch *WebSocketChannel) Close() {
	ch.conn.Close()
}
