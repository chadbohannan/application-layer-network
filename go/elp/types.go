package elp

const ( // Link State enumeration
	// LinkStateRouteQuery is a channel broadcast requesting LinkStateNodeRouteCost responses
	LinkStateRouteQuery = 1

	// LinkStateNodeRouteCost data is AddressType length + 2 bytes for int cost value
	LinkStateNodeRouteCost = 2
)

func readINT16U(buff []byte) uint16 {
	if len(buff) == 2 {
		return (uint16(buff[0]) << 8) | uint16(buff[1])
	}
	return 0xFFFF
}

func writeINT16U(value uint16) []byte {
	return []byte{
		byte(value >> 8),
		byte(value & 0xFF),
	}
}

func readINT32U(buff []byte) uint32 {
	if len(buff) == 4 {
		return uint32(buff[0])<<24 |
			uint32(buff[1])<<16 |
			uint32(buff[2])<<8 |
			uint32(buff[3])
	}
	return 0xFFFFFFFF
}

func writeINT32U(value uint32) []byte {
	return []byte{
		byte((value >> 24) & 0xFF),
		byte((value >> 16) & 0xFF),
		byte((value >> 8) & 0xFF),
		byte(value & 0xFF),
	}
}
