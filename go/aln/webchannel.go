package aln

import (
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
	p.DataSize = uint16(len(p.Data))
	// fmt.Printf("send to %s from %s via:%s net:%d ctxID:%d service:'%s' data:%v sz:%d\n",
	// 	p.DestAddr, p.SrcAddr, p.NextAddr, p.NetState, p.ContextID, p.Service, p.Data, p.DataSize)

	ch.mutex.Lock()
	defer ch.mutex.Unlock()
	return ch.conn.WriteJSON(p)
}

// Receive deserializes packets from it's socket
func (ch *WebSocketChannel) Receive(onPacket PacketCallback, onClose OnCloseCallback) {
	ch.conn.SetCloseHandler(func(code int, text string) error {
		log.Printf("WebSocketChannel:onCloseHandler")
		return nil
	})
	for { // ever and ever
		packet := &Packet{}
		if err := ch.conn.ReadJSON(packet); err == nil {
			onPacket(packet)
		} else {
			log.Printf("client read err: %s\nclosing socket: %s", err.Error(), ch.conn.RemoteAddr().String())
			break // jk; not forever
		}
	}
	log.Printf("closing websocket")
	onClose(ch)
}

// Close .
func (ch *WebSocketChannel) Close() {
	ch.conn.Close()
}
