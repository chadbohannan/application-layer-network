package aln

func bytesToINT16U(buff []byte) uint16 {
	if len(buff) == 2 {
		return (uint16(buff[0]) << 8) | uint16(buff[1])
	}
	return 0xFFFF
}

func bytesOfINT16U(value uint16) []byte {
	return []byte{
		byte(value >> 8),
		byte(value & 0xFF),
	}
}

func bytesOfAddressType(value AddressType) []byte {
	return []byte(value)
}

func bytesToAddressType(buff []byte) AddressType {
	return AddressType(buff)
}

func bytesToINT32U(buff []byte) uint32 {
	if len(buff) == 4 {
		return uint32(buff[0])<<24 |
			uint32(buff[1])<<16 |
			uint32(buff[2])<<8 |
			uint32(buff[3])
	}
	return 0xFFFFFFFF
}

func bytesOfINT32U(value uint32) []byte {
	return []byte{
		byte((value >> 24) & 0xFF),
		byte((value >> 16) & 0xFF),
		byte((value >> 8) & 0xFF),
		byte(value & 0xFF),
	}
}
