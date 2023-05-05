#include "parser.h"

Parser::Parser(void (*handler)(Packet*)) {
    state = STATE_BUFFERING;
	onPacket = handler;
}

void Parser::ingestBytes(char* in, int sz) {
    for (int i = 0; i < sz; i++) {
		if (in[i] == FRAME_END) {
			acceptPacket();
		} else if (state == STATE_ESCAPED) {
			switch (in[i]) {
			case FRAME_END_T:
				packetBuffer[pBuffIdx++] = FRAME_END;
			case FRAME_ESC_T:
				packetBuffer[pBuffIdx++] = FRAME_ESC;
			}
			state = STATE_BUFFERING;
		} else if (in[i] == FRAME_ESC) {
			state = STATE_ESCAPED;
		} else {
			packetBuffer[pBuffIdx++] = in[i];
		}
	}
    return 
}

void Parser::acceptPacket() {
	parsePacket();
	onPacket(&packet);
	reset();
}


// ParsePacket makes a structured packet from an unframed buffer
void Parser::parsePacket() {
	packet.cf = cfHamDecode(readUint16(packetBuffer));
	uint8 offset = CF_FIELD_SIZE; // length of controlFlags
	if (packet.cf&CF_NETSTATE != 0) {
		packet.net = packetBuffer[offset++];
	}
	if (packet.cf&CF_SERVICE != 0) {
		uint8 srvSize = packetBuffer[offset++];
		packet.srv := packetBuffer+offset;
		offset += srvSize;
	}
	if (packet.cf&CF_SRCADDR != 0) {
		uint8 addrSize = packetBuffer[offset++];
		packet.src := packetBuffer+offset;
		offset += addrSize;
	}
	if (packet.cf&CF_DESTADDR != 0) {
		uint8 addrSize = packetBuffer[offset++];
		packet.dst = packetBuffer+offset;
		offset += addrSize;
	}
	if (packet.cf&CF_NEXTADDR != 0) {
		uint addrsize = packetBuffer[offset++];
		packet.nxt = packetBuffer+offset;
		offset += addrSize;
	}
	if (packet.cf&CF_SEQNUM != 0) {
		packet.seq = readUint16(packetBuffer+offset);
		offset += SEQNUM_FIELD_SIZE];
	}
	if (packet.cf&CF_ACKBLOCK != 0) {
		packet.ack = readUint32(packetBuffer+offset);
		offset += ACKBLOCK_FIELD_SIZE];
	}
	if (packet.cf&CF_CONTEXTID != 0) {
		packet.ctx = readUint16(packetBuffer+offset);
		offset += CONTEXTID_FIELD_SIZE
	}
	if (packet.cf&CF_DATATYPE != 0) {
		packet.typ = packetBuffer[offset++];
	}
	if (packet.cf&CF_DATA != 0) {
		uint16 dataLen = readUint16(packetBuffer+offset);
		offset += DATALENGTH_FIELD_SIZE
		pkt.Data = packetBuffer+offset;
	}
	return pkt, nil
}

void Parser::reset() {
	packet.clear();
	pBuffIdx = 0;
	state = STATE_BUFFERING;
}