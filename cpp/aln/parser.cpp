#include <stdio.h>
#include "parser.h"

Parser::Parser(void (*handler)(Packet*)) {
    onPacket = handler;
	reset();
}

void Parser::ingestFrameBytes(uint8* in, int sz) {
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
	if (packet.cf&CF_NETSTATE) {
		printf("net\n");
		packet.net = packetBuffer[offset++];
	}
	if (packet.cf&CF_SERVICE) {
		printf("srv\n");
		packet.srvSz = packetBuffer[offset++];
		packet.srv = packetBuffer+offset;
		offset += packet.srvSz;
	}
	if (packet.cf&CF_SRCADDR) {
		printf("src\n");
		uint8 addrSize = packetBuffer[offset++];
		packet.src = packetBuffer+offset;
		offset += addrSize;
	}
	if (packet.cf&CF_DESTADDR) {
		printf("dst, offset:%d\n", offset);
		packet.dstSz = packetBuffer[offset++];
		printf("dst, addrSize:%d\n", packet.dstSz);
		packet.dst = packetBuffer+offset;
		printf("dst:%.*s\n", packet.dstSz, packet.dst);
		offset += packet.dstSz;
		printf("offset:%d\n", offset);
	}
	if (packet.cf&CF_NEXTADDR) {
		printf("nxt\n");
		packet.nxtSz = packetBuffer[offset++];
		packet.nxt = packetBuffer+offset;
		offset += packet.nxtSz;
	}
	if (packet.cf&CF_SEQNUM) {
		printf("seq\n");
		packet.seq = readUint16(packetBuffer+offset);
		offset += SEQNUM_FIELD_SIZE;
	}
	if (packet.cf&CF_ACKBLOCK) {
		printf("ack\n");
		packet.ack = readUint32(packetBuffer+offset);
		offset += ACKBLOCK_FIELD_SIZE;
	}
	if (packet.cf&CF_CONTEXTID) {
		printf("ctx\n");
		packet.ctx = readUint16(packetBuffer+offset);
		offset += CONTEXTID_FIELD_SIZE;
	}
	if (packet.cf&CF_DATATYPE) {
		printf("typ\n");
		packet.typ = packetBuffer[offset++];
	}
	if (packet.cf&CF_DATA) {
		printf("data\n");
		packet.dataSz = readUint16(packetBuffer+offset);
		offset += DATALENGTH_FIELD_SIZE;
		packet.data = packetBuffer+offset;
	}
}

void Parser::reset() {
	packet.clear();
	pBuffIdx = 0;
	state = STATE_BUFFERING;
}