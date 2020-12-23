package elp

import "io"

// AddressType of all ELP packets
type AddressType uint16

// AddressTypeSize must be the size of AddressType in bytes
const AddressTypeSize = 2

// Packet framing
const (
	FRAME_LEADER_LENGTH = 4
	FRAME_CF_LENGTH     = 2
	FRAME_LEADER        = 0x3C // '<' (ASCII)
	FRAME_ESCAPE        = 0xC3 // 'Ã' (Extended ASCII)
)

// Control Flag bits (Hamming encoding consumes 5 bits, leaving 11)
const (
	CF_UNUSED2   = 0x0400
	CF_UNUSED1   = 0x0200
	CF_LINKSTATE = 0x0100
	CF_SRCADDR   = 0x0080
	CF_DESTADDR  = 0x0040
	CF_SEQNUM    = 0x0020
	CF_ACKBLOCK  = 0x0010
	CF_CONTEXTID = 0x0008
	CF_DATATYPE  = 0x0004
	CF_DATA      = 0x0002
	CF_CRC       = 0x0001
)

// Packet header field sizes
const (
	CF_FIELD_SIZE         = 2 // INT16U
	LINKSTATE_FIELD_SIZE  = 1 // INT08U enumerated
	SRCADDR_FIELD_SIZE    = 2 // INT16U
	DESTADDR_FIELD_SIZE   = 2 // INT16U
	SEQNUM_FIELD_SIZE     = 2 // INT16U
	ACKBLOCK_FIELD_SIZE   = 4 // INT32U
	DATALENGTH_FIELD_SIZE = 2 // INT16U
	CRC_FIELD_SIZE        = 4 // INT32U
)

// limits
const (
	MAX_HEADER_SIZE = CF_FIELD_SIZE +
		LINKSTATE_FIELD_SIZE +
		SRCADDR_FIELD_SIZE +
		DESTADDR_FIELD_SIZE +
		SEQNUM_FIELD_SIZE +
		ACKBLOCK_FIELD_SIZE +
		DATALENGTH_FIELD_SIZE
	MAX_DATA_SIZE   = 1024 // small number to prevent stack overflow
	MAX_PACKET_SIZE = MAX_HEADER_SIZE + MAX_DATA_SIZE + CRC_FIELD_SIZE
)

type Packet struct {
	ControlFlags uint16
	LinkState    byte
	SrcAddr      AddressType
	DestAddr     AddressType
	SeqNum       uint16
	AckBlock     uint32
	DataSize     uint16
	Data         []byte
	CrcSum       uint32
}

func NewPacket() *Packet {
	return &Packet{}
}

func ParsePacket(buf []byte) (*Packet, error) {
	return nil, nil
}

func headerLength(controlFlags uint16) int {
	return 0
}

func headerFieldOffset(controlFlags, fieldBit uint16) int {
	return 0
}

func (p *Packet) HeaderLength() int {
	return headerLength(p.ControlFlags)
}

// returns an unframed packet buffer
func (p *Packet) ToBytes() (output []byte) {
	return output
}

// returns a byte array with starting delimiter and may contain escape chars
func (p *Packet) ToFrameBytes() (output []byte) {
	return output
}

// writes a framed packet to the writer
func (p *Packet) WriteTo(w io.Writer) (n int64, err error) {
	return n, err
}

func CRC32(pdata []byte) uint32 {
	return 0
}

/*
 * This encodes the CF using a modified Hamming (15,11).
 * Returns the encoded 11 bit CF as a 15 bit codeword.
 * Only the 0x07FF bits are  aloud to be on for the input all others will be ignored.
 * Based off of the following Hamming (15,11) matrix...
 * G[16,11] = [[1000,0000,0000,1111],  0x800F
 *             [0100,0000,0000,0111],  0x4007
 *             [0010,0000,0000,1011],  0x200B
 *             [0001,0000,0000,1101],  0x100D
 *             [0000,1000,0000,1110],  0x080E
 *             [0000,0100,0000,0011],  0x0403
 *             [0000,0010,0000,0101],  0x0205
 *             [0000,0001,0000,0110],  0x0106
 *             [0000,0000,1000,1010],  0x008A
 *             [0000,0000,0100,1001],  0x0049
 *             [0000,0000,0010,1100]]; 0x002C
 */
func CFHamEncode(value uint16) uint16 {
	return 0
}

/*
 * This decodes the CF using a modified Hamming (15,11).
 * It will fix one error, if only one error occures, not very good for burst errors.
 * This is a SEC (single error correction) which means it has no BED (bit error detection) to save on size.
 * The returned value will be, if fixed properly, the same 11 bits that were sent into the encoder.
 * Bits 0xF800 will be zero.
 * Based off of the following Hamming (15,11) matrix...
 * H[16,4]  = [[1011,1000,1110,1000],  0x171D
 *             [1101,1011,0010,0100],  0x24DB
 *             [1110,1101,1000,0010],  0x41B7
 *             [1111,0110,0100,0001]]; 0x826F
 */
func CFHamDecode(value uint16) uint16 {
	return 0
}

/* DESCRIPTION : This is a modified "Sparse Ones Bit Count".
 *               Instead of counting the ones it just determines
 *               if there was an odd or even count of bits by XORing the int.
 * ARGUMENTS   : This is a 32 bit number to count the ones of.
 * RETURNS     : Return 0x0 or 0x1. */
func IntXOR(n uint32) byte {
	return 0
}
