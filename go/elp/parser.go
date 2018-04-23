package elp

// Parser buffers a byte sequence and emits packets as they are read
// from a an input stream.

// Packet parsing state enumeration
const (
	STATE_FINDSTART = 0
	STATE_GET_CF    = 1
	STATE_GETHEADER = 2
	STATE_GETDATA   = 3
	STATE_GETCRC    = 4
)

// LinkState value enumerations (TODO support mesh routing)
const (
	LINK_CONNECT   = 0x01
	LINK_CONNECTED = 0x03
	LINK_PING      = 0x05
	LINK_PONG      = 0x07
	LINK_ACKRESEND = 0x09
	LINK_NOACK     = 0x0B
	LINK_CLOSE     = 0x0D
)

type PacketCallback func(*Packet)

type Parser struct {
	frameBuffer []byte // clears on frame delimiter; contains no esc chars

	state        byte   // enumerated State
	delimCount   uint8  // counts '<' chars to detect new frames
	controlFlags uint16 // hamming-decoded first 2 bytes of frameBuffer

	headerIndex  uint8 // offset into header of next header byte
	headerLength uint8

	dataIndex  uint16 // offset into data of next data byte
	dataLength uint16 // local-hardware decoded datalength value
	data       []byte // slice from framebuffer of data

	crcIndex  uint8  // offset into CRC of next CRC byte
	crcLength uint8  // 4; only CRC32 is supported
	crcSum    []byte // slice of frameBuffer over which CRC is computed

	packet_callback PacketCallback
}

func NewParser(cb PacketCallback) *Parser {
	return &Parser{
		packet_callback: cb,
	}
}

func (p *Parser) acceptPacket() {
	// TODO call callback and clear state
}

// Principle method for consuming input from a stream
func (p *Parser) ParseBytes(buffer []byte) {
	// TODO
}
