package aln

import (
	"bytes"
	"fmt"
)

// Parser buffers a byte sequence and emits packets as they are read
// from a an input stream.

// Packet parsing state enumeration
const (
	STATE_BUFFERING = 0
	STATE_ESCAPED   = 1
)

type PacketCallback func(*Packet) bool
type OnCloseCallback func(Channel)

type Parser struct {
	buffer         *bytes.Buffer
	state          byte // enumerated State
	packetCallback PacketCallback
}

func NewParser(cb PacketCallback) *Parser {
	return &Parser{
		state:          STATE_BUFFERING,
		buffer:         bytes.NewBuffer([]byte{}),
		packetCallback: cb,
	}
}

// Clear resets the state of the parser following a frame
func (p *Parser) Clear() {
	p.buffer.Reset()
	p.state = STATE_BUFFERING
}

func (p *Parser) acceptPacket() bool {
	defer p.Clear()
	if pkt, err := ParsePacket(p.buffer.Bytes()); err == nil {
		return p.packetCallback(pkt)
	} else {
		fmt.Println("on acceptPacket, ParsePacket:" + err.Error())
	}
	return true
}

func (p *Parser) IngestStream(buffer []byte) bool {
	for _, msg := range buffer {
		if msg == FRAME_END {
			if !p.acceptPacket() {
				return false
			}
		} else if p.state == STATE_ESCAPED {
			switch msg {
			case FRAME_END_T:
				p.buffer.WriteByte(FRAME_END)
			case FRAME_ESC_T:
				p.buffer.WriteByte(FRAME_ESC)
			default:
				fmt.Printf("unexpected char in KISS frame: %x", msg)
			}
			p.state = STATE_BUFFERING
		} else if msg == FRAME_ESC {
			p.state = STATE_ESCAPED
		} else {
			p.buffer.WriteByte(msg)
		}
	}
	return true
}
