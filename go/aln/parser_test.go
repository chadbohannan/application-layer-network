package aln

import (
	"testing"
)

func TestINT16U(t *testing.T) {
	expected := uint16(0x0102)
	buff := bytesOfINT16U(expected)
	result := bytesToINT16U(buff)
	if expected != result {
		t.Fatalf("%d != %d, %v", expected, result, buff)
	}
}

func TestINT32U(t *testing.T) {
	expected := uint32(0x01020304)
	buff := bytesOfINT32U(expected)
	result := bytesToINT32U(buff)
	if expected != result {
		t.Fatalf("%d != %d, %v", expected, result, buff)
	}
}

func TestParser(t *testing.T) {
	pkt0 := NewPacket()
	pkt0.DestAddr = 0x0101
	pkt0.Data = []byte("test")
	buff, err := pkt0.ToBytes()
	if err != nil {
		t.Fatalf("pkt.ToBytes err: %s", err.Error())
	}

	pkt1, err := ParsePacket(buff)
	if err != nil {
		t.Fatalf("ParsePacket err: %s", err.Error())
	}
	if pkt1.DestAddr != pkt0.DestAddr {
		t.Fatalf("expected DestAddress = %d, got %d", pkt0.DestAddr, pkt1.DestAddr)
	}
	if len(pkt1.Data) != 4 {
		t.Fatalf("expected DataLength = %d, got %d", 4, len(pkt1.Data))
	}
	if string(pkt1.Data) != "test" {
		t.Fatalf("expected Data = 'test, got %s", string(pkt1.Data))
	}
}

func TestParseFrame(t *testing.T) {
	pkt := NewPacket()
	pkt.NetState = 1
	pkt.Data = []byte("ABCD")
	pktFrame, err := pkt.ToFrameBytes()
	if err != nil {
		t.Fatal(err)
	}

	// parse the packet fram into a new packet body
	var p *Packet
	parser := NewParser(func(q *Packet) {
		p = q
	})
	parser.IngestStream(pktFrame)

	// assert the packet was received
	if p == nil {
		t.Fatalf("failed to parse packet")
	}
	if p.DataSize != 4 {
		t.Fatalf("pkt: %s", p.ToJsonString())
	}
}
