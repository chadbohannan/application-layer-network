package aln

// CRC32 is a 32 bit Cyclic Redundancy Check
func CRC32(pdata []byte) uint32 {
	var bit uint32
	crc := uint32(0xFFFFFFFF)
	for _, c := range pdata {
		j := 0x0001
		for j <= 0x0080 {
			bit = crc & 0x80000000
			crc <<= 1
			if (int(c) & j) > 0 {
				bit ^= 0x80000000
			}
			if bit > 0 {
				crc ^= 0x4C11DB7
			}
			j <<= 1
		}
	}
	// reverse
	crc = (((crc & 0x55555555) << 1) | ((crc & 0xAAAAAAAA) >> 1))
	crc = (((crc & 0x33333333) << 2) | ((crc & 0xCCCCCCCC) >> 2))
	crc = (((crc & 0x0F0F0F0F) << 4) | ((crc & 0xF0F0F0F0) >> 4))
	crc = (((crc & 0x00FF00FF) << 8) | ((crc & 0xFF00FF00) >> 8))
	crc = (((crc & 0x0000FFFF) << 16) | ((crc & 0xFFFF0000) >> 16))
	return (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF
}

// CFHamEncode encodes the CF using a modified Hamming (15,11).
/* Returns the encoded 11 bit CF as a 15 bit codeword.
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
	return (value & 0x07FF) |
		(IntXOR(value&0x071D) << 12) |
		(IntXOR(value&0x04DB) << 13) |
		(IntXOR(value&0x01B7) << 14) |
		(IntXOR(value&0x026F) << 15)
}

// CFHamDecode decodes the CF using a modified Hamming (15,11).
/* It will fix one error, if only one error occures, not very good for burst errors.
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
	hamDecodMap := map[uint8]uint16{
		0x0F: 0x0001,
		0x07: 0x0002,
		0x0B: 0x0004,
		0x0D: 0x0008,
		0x0E: 0x0010,
		0x03: 0x0020,
		0x05: 0x0040,
		0x06: 0x0080,
		0x0A: 0x0100,
		0x09: 0x0200,
		0x0C: 0x0400,
	}
	e := IntXOR(value&0x826F) |
		(IntXOR(value&0x41B7) << 1) |
		(IntXOR(value&0x24DB) << 2) |
		(IntXOR(value&0x171D)<<3)&
			0xFF // mask into single byte
	value ^= hamDecodMap[uint8(e)]
	return value
}

// IntXOR is a modified "Sparse Ones Bit Count".
/*               Instead of counting the ones it just determines
 *               if there was an odd or even count of bits by XORing the int.
 * ARGUMENTS   : This is a 32 bit number to count the ones of.
 * RETURNS     : Return 0x0 or 0x1. */
func IntXOR(n uint16) uint16 {
	var cnt uint16
	for n != 0 { // This loop will only execute the number times equal to the number of ones. */
		cnt ^= 0x1
		n &= (n - 0x1)
	}
	return cnt
}
