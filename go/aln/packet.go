package aln

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
)

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
	CF_HAMMING1  = 0x8000
	CF_HAMMING2  = 0x4000
	CF_HAMMING3  = 0x2000
	CF_HAMMING4  = 0x1000
	CF_HAMMING5  = 0x0800
	CF_NETSTATE  = 0x0400
	CF_SERVICEID = 0x0200
	CF_SRCADDR   = 0x0100
	CF_DESTADDR  = 0x0080
	CF_NEXTADDR  = 0x0040
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
	NETSTATE_FIELD_SIZE   = 1 // INT08U enumerated
	SERVICEID_FIELD_SIZE  = 2 // INT16U enumerated
	SRCADDR_FIELD_SIZE    = 2 // INT16U
	DESTADDR_FIELD_SIZE   = 2 // INT16U
	NEXTADDR_FIELD_SIZE   = 2 // INT16U
	SEQNUM_FIELD_SIZE     = 2 // INT16U
	ACKBLOCK_FIELD_SIZE   = 4 // INT32U
	CONTEXTID_FIELD_SIZE  = 2 // INT16U
	DATATYPE_FIELD_SIZE   = 1 // INT08U
	DATALENGTH_FIELD_SIZE = 2 // INT16U
	CRC_FIELD_SIZE        = 4 // INT32U
)

// limits
const (
	MAX_HEADER_SIZE = CF_FIELD_SIZE +
		NETSTATE_FIELD_SIZE +
		SERVICEID_FIELD_SIZE +
		SRCADDR_FIELD_SIZE +
		DESTADDR_FIELD_SIZE +
		NEXTADDR_FIELD_SIZE +
		SEQNUM_FIELD_SIZE +
		ACKBLOCK_FIELD_SIZE +
		CONTEXTID_FIELD_SIZE +
		DATATYPE_FIELD_SIZE +
		DATALENGTH_FIELD_SIZE
	MAX_DATA_SIZE   = 1024 // small number to prevent stack overflow
	MAX_PACKET_SIZE = MAX_HEADER_SIZE + MAX_DATA_SIZE + CRC_FIELD_SIZE
)

type Packet struct {
	ControlFlags uint16      `bson:"cf" json:"cf"`
	NetState     byte        `bson:"net" json:"net"`
	ServiceID    uint16      `bson:"srv" json:"srv"`
	SrcAddr      AddressType `bson:"src" json:"src"`
	DestAddr     AddressType `bson:"dst" json:"dst"`
	NextAddr     AddressType `bson:"nxt" json:"nxt"`
	SeqNum       uint16      `bson:"seq" json:"seq"`
	AckBlock     uint32      `bson:"ack" json:"ack"`
	ContextID    uint16      `bson:"ctx" json:"ctx"`
	DataType     uint8       `bson:"typ" json:"typ"`
	DataSize     uint16      `bson:"sz" json:"sz"`
	Data         []byte      `bson:"data" json:"data"`
	CrcSum       uint32      `bson:"crc" json:"crc"`
}

func NewPacket() *Packet {
	return &Packet{}
}

// ParsePacket makes a structured packet from an unframed buffer
func ParsePacket(packetBuffer []byte) (*Packet, error) {
	if len(packetBuffer) < 2 {
		return nil, fmt.Errorf("packet < 2 bytes long")
	}

	pkt := NewPacket()

	cfBytes := packetBuffer[0:CF_FIELD_SIZE]
	pkt.ControlFlags = CFHamDecode(bytesToINT16U(cfBytes))
	offset := CF_FIELD_SIZE // length of controlFlags
	if pkt.ControlFlags&CF_NETSTATE != 0 {
		pkt.NetState = packetBuffer[offset]
		offset += NETSTATE_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_SERVICEID != 0 {
		seqBytes := packetBuffer[offset : offset+SERVICEID_FIELD_SIZE]
		pkt.ServiceID = bytesToINT16U(seqBytes)
		offset += SERVICEID_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_SRCADDR != 0 {
		srcBytes := packetBuffer[offset : offset+SRCADDR_FIELD_SIZE]
		pkt.SrcAddr = bytesToAddressType(srcBytes)
		offset += SRCADDR_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_DESTADDR != 0 {
		destBytes := packetBuffer[offset : offset+DESTADDR_FIELD_SIZE]
		pkt.DestAddr = bytesToAddressType(destBytes)
		offset += DESTADDR_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_NEXTADDR != 0 {
		destBytes := packetBuffer[offset : offset+NEXTADDR_FIELD_SIZE]
		pkt.NextAddr = bytesToAddressType(destBytes)
		offset += NEXTADDR_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_SEQNUM != 0 {
		seqBytes := packetBuffer[offset : offset+SEQNUM_FIELD_SIZE]
		pkt.SeqNum = bytesToINT16U(seqBytes)
		offset += SEQNUM_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_ACKBLOCK != 0 {
		ackBytes := packetBuffer[offset : offset+ACKBLOCK_FIELD_SIZE]
		pkt.AckBlock = bytesToINT32U(ackBytes)
		offset += ACKBLOCK_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_CONTEXTID != 0 {
		contextBytes := packetBuffer[offset : offset+CONTEXTID_FIELD_SIZE]
		pkt.ContextID = bytesToINT16U(contextBytes)
		offset += CONTEXTID_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_DATATYPE != 0 {
		pkt.DataType = packetBuffer[offset]
		offset += DATATYPE_FIELD_SIZE
	}
	if pkt.ControlFlags&CF_DATA != 0 {
		lengthBytes := packetBuffer[offset : offset+DATALENGTH_FIELD_SIZE]
		pkt.DataSize = bytesToINT16U(lengthBytes)
		offset += DATALENGTH_FIELD_SIZE
		pkt.Data = packetBuffer[offset : offset+int(pkt.DataSize)]
	}
	return pkt, nil
}

func HeaderLength(controlFlags uint16) uint8 {
	return HeaderFieldOffset(controlFlags, 0)
}

func HeaderFieldOffset(controlFlags uint16, field uint16) uint8 {
	offset := uint8(CF_FIELD_SIZE)
	if field == CF_NETSTATE {
		return offset
	}
	if controlFlags&CF_NETSTATE != 0 {
		offset += NETSTATE_FIELD_SIZE
	}
	if field == CF_SERVICEID {
		return offset
	}
	if controlFlags&CF_SERVICEID != 0 {
		offset += SERVICEID_FIELD_SIZE
	}
	if field == CF_SRCADDR {
		return offset
	}
	if controlFlags&CF_SRCADDR != 0 {
		offset += SRCADDR_FIELD_SIZE
	}
	if field == CF_DESTADDR {
		return offset
	}
	if controlFlags&CF_DESTADDR != 0 {
		offset += DESTADDR_FIELD_SIZE
	}
	if field == CF_NEXTADDR {
		return offset
	}
	if controlFlags&CF_NEXTADDR != 0 {
		offset += NEXTADDR_FIELD_SIZE
	}
	if field == CF_SEQNUM {
		return offset
	}
	if controlFlags&CF_SEQNUM != 0 {
		offset += SEQNUM_FIELD_SIZE
	}
	if field == CF_ACKBLOCK {
		return offset
	}
	if controlFlags&CF_ACKBLOCK != 0 {
		offset += ACKBLOCK_FIELD_SIZE
	}
	if field == CF_CONTEXTID {
		return offset
	}
	if controlFlags&CF_CONTEXTID != 0 {
		offset += CONTEXTID_FIELD_SIZE
	}
	if field == CF_DATATYPE {
		return offset
	}
	if controlFlags&CF_DATATYPE != 0 {
		offset += DATATYPE_FIELD_SIZE
	}
	if field == CF_DATA {
		return offset
	}
	if controlFlags&CF_DATA != 0 {
		offset += DATALENGTH_FIELD_SIZE
	}
	return offset
}

func (p *Packet) SetControlFlags() {
	controlFlags := uint16(0)
	p.DataSize = uint16(len(p.Data))
	if p.NetState != 0 {
		controlFlags |= CF_NETSTATE
	}
	if p.ServiceID != 0 {
		controlFlags |= CF_SERVICEID
	}
	if p.SrcAddr != 0 {
		controlFlags |= CF_SRCADDR
	}
	if p.DestAddr != 0 {
		controlFlags |= CF_DESTADDR
	}
	if p.NextAddr != 0 {
		controlFlags |= CF_NEXTADDR
	}
	if p.SeqNum != 0 {
		controlFlags |= CF_SEQNUM
	}
	if p.AckBlock != 0 {
		controlFlags |= CF_ACKBLOCK
	}
	if p.ContextID != 0 {
		controlFlags |= CF_CONTEXTID
	}
	if p.DataType != 0 {
		controlFlags |= CF_DATATYPE
	}
	if p.DataSize != 0 {
		controlFlags |= CF_DATA
	}

	p.ControlFlags = CFHamEncode(controlFlags)
}

// ToBytes returns an unframed packet buffer
func (p *Packet) ToBytes() ([]byte, error) {
	if len(p.Data) > 65535 {
		return nil, fmt.Errorf("len p.Data > 65535")
	}
	p.DataSize = uint16(len(p.Data))
	p.SetControlFlags()

	buff := bytes.NewBuffer([]byte{})
	buff.Write(bytesOfINT16U(p.ControlFlags))

	if p.ControlFlags&CF_NETSTATE != 0 {
		buff.Write([]byte{p.NetState})
	}
	if p.ControlFlags&CF_SERVICEID != 0 {
		buff.Write(bytesOfINT16U(p.ServiceID))
	}
	if p.ControlFlags&CF_SRCADDR != 0 {
		buff.Write(bytesOfAddressType(p.SrcAddr))
	}
	if p.ControlFlags&CF_DESTADDR != 0 {
		buff.Write(bytesOfAddressType(p.DestAddr))
	}
	if p.ControlFlags&CF_NEXTADDR != 0 {
		buff.Write(bytesOfAddressType(p.NextAddr))
	}
	if p.ControlFlags&CF_SEQNUM != 0 {
		buff.Write(bytesOfINT16U(p.SeqNum))
	}
	if p.ControlFlags&CF_ACKBLOCK != 0 {
		buff.Write(bytesOfINT32U(p.AckBlock))
	}
	if p.ControlFlags&CF_CONTEXTID != 0 {
		buff.Write(bytesOfINT16U(p.ContextID))
	}
	if p.ControlFlags&CF_DATATYPE != 0 {
		buff.Write([]byte{p.DataType})
	}
	if p.ControlFlags&CF_DATA != 0 {
		buff.Write(bytesOfINT16U(p.DataSize))
		buff.Write(p.Data)
	}
	if p.ControlFlags&CF_CRC != 0 {
		p.CrcSum = CRC32(buff.Bytes())
		buff.Write(bytesOfINT32U(p.CrcSum))
	}
	return buff.Bytes(), nil
}

// ToFrameBytes returns a byte array with starting delimiter and may contain escape chars
func (p *Packet) ToFrameBytes() ([]byte, error) {
	buff := bytes.NewBuffer([]byte{})
	for i := 0; i < FRAME_LEADER_LENGTH; i++ {
		buff.WriteByte(FRAME_LEADER)
	}
	packetBytes, err := p.ToBytes()
	if err != nil {
		return nil, err
	}
	delimCount := 0
	for _, byt := range packetBytes {
		buff.WriteByte(byt)
		if byt == FRAME_LEADER {
			delimCount++
		} else {
			delimCount = 0
		}
		if delimCount == (FRAME_LEADER_LENGTH - 1) {
			buff.WriteByte(FRAME_ESCAPE)
			delimCount = 0
		}
	}

	return buff.Bytes(), nil
}

// WriteTo writes a framed packet to the writer
func (p *Packet) WriteTo(w io.Writer) (n int, err error) {
	if byts, err := p.ToFrameBytes(); err == nil {
		return w.Write(byts)
	} else {
		return 0, err
	}
}

func (p *Packet) ToJsonString() string {
	byts, _ := json.MarshalIndent(p, "", " ")
	return string(byts)
}
