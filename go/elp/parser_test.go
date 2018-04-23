package elp

import "testing"

func TestINT16U(t *testing.T) {
	expected := uint16(0x0102)
	buff := writeINT16U(expected)
	result := readINT16U(buff)
	if expected != result {
		t.Fatalf("%d != %d, %v", expected, result, buff)
	}
}

func TestINT32U(t *testing.T) {
	expected := uint32(0x01020304)
	buff := writeINT32U(expected)
	result := readINT32U(buff)
	if expected != result {
		t.Fatalf("%d != %d, %v", expected, result, buff)
	}
}

func TestParser(t *testing.T) {
	// TODO create Packet
	// TODO serialize Packet
	// TODO parse Packet
}
