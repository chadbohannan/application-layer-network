package aln

import (
	"fmt"
	"sync"

	"github.com/gorilla/websocket"
)

type WebSocketChannel struct {
	mutex sync.Mutex
	conn  *websocket.Conn
	onCloseOnce  sync.Once
	onCloseHandler OnCloseCallback
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
		fmt.Println("WebSocketChannel:onCloseHandler")
		return nil
	})
	for { // ever and ever
		packet := &Packet{}
		if err := ch.conn.ReadJSON(packet); err == nil {
			onPacket(packet)
		} else {
			fmt.Printf("client read err: %s\nclosing socket: %s\n", err.Error(), ch.conn.RemoteAddr().String())
			break // jk; not forever
		}
	}
	fmt.Println("closing websocket")
	// ch.Close() // Removed redundant call
}

// Close .
func (ch *WebSocketChannel) Close() {
	ch.conn.Close()
	ch.onCloseOnce.Do(func() {
		if ch.onCloseHandler != nil {
			ch.onCloseHandler(ch)
		}
	})
}

// OnClose registers handlers to be notified of channel teardown
func (ch *WebSocketChannel) OnClose(onClose OnCloseCallback) {
	ch.onCloseHandler = onClose
}
