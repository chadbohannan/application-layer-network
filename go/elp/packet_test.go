package elp

import "testing"

func TestCRC32(t *testing.T) {
	res := CRC32([]byte{1, 2, 3, 4})
	if res != 3057449933 {
		t.Fatalf("Expected 3057449933, got %d", res)
	}
}
