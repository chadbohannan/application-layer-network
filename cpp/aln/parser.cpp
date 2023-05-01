#include "parser.h"

Parser::Parser(void (*handler)(*Packet)) {
    onPacket = handler;
}

void Parser:ingestBytes(char* in, int sz) {
    for (int i = 0; i < sz; i++) {
		if in[i] == FRAME_END {
			accept();
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
    return 
}

bool Parser::acceptPacket() {
	defer p.Clear()
	if pkt, err := ParsePacket(p.buffer.Bytes()); err == nil {
		return p.packetCallback(pkt)
	} else {
		fmt.Println("on acceptPacket, ParsePacket:" + err.Error())
	}
	return true
}


// ParsePacket makes a structured packet from an unframed buffer
Parser::parsePacket() (*Packet, error) {
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
	if pkt.ControlFlags&CF_SERVICE != 0 {
		srvSize := int(packetBuffer[offset])
		offset += 1
		seqBytes := packetBuffer[offset : offset+srvSize]
		pkt.Service = string(seqBytes)
		offset += srvSize
	}
	if pkt.ControlFlags&CF_SRCADDR != 0 {
		addrSize := int(packetBuffer[offset])
		offset += 1
		srcBytes := packetBuffer[offset : offset+addrSize]
		pkt.SrcAddr = bytesToAddressType(srcBytes)
		offset += addrSize
	}
	if pkt.ControlFlags&CF_DESTADDR != 0 {
		addrSize := int(packetBuffer[offset])
		offset += 1
		destBytes := packetBuffer[offset : offset+addrSize]
		pkt.DestAddr = bytesToAddressType(destBytes)
		offset += addrSize
	}
	if pkt.ControlFlags&CF_NEXTADDR != 0 {
		addrSize := int(packetBuffer[offset])
		offset += 1
		nextBytes := packetBuffer[offset : offset+addrSize]
		pkt.NextAddr = bytesToAddressType(nextBytes)
		offset += addrSize
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
