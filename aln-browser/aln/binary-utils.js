// Native browser API utilities for binary data handling
// Replaces ByteBuffer dependency with zero-dependency implementation

// Text encoding/decoding
const encoder = new TextEncoder();
const decoder = new TextDecoder();

// ============================================================================
// Base64 Encoding/Decoding
// ============================================================================

/**
 * Convert base64 string to binary string
 * @param {string} base64 - Base64 encoded string
 * @returns {string} Binary string
 */
export function base64ToBinary(base64) {
  const binaryString = atob(base64);
  return binaryString;
}

/**
 * Convert binary string to base64
 * @param {string} binaryString - Binary string
 * @returns {string} Base64 encoded string
 */
export function binaryToBase64(binaryString) {
  return btoa(binaryString);
}

// ============================================================================
// Binary Packet Builder (replaces new ByteBuffer())
// ============================================================================

export class BinaryBuilder {
  constructor(capacity = 256) {
    this.buffer = new ArrayBuffer(capacity);
    this.view = new DataView(this.buffer);
    this.bytes = new Uint8Array(this.buffer);
    this.offset = 0;
  }

  writeUint8(value) {
    this._ensureCapacity(1);
    this.view.setUint8(this.offset, value);
    this.offset += 1;
    return this;
  }

  writeUint16(value) {
    this._ensureCapacity(2);
    this.view.setUint16(this.offset, value, false); // big-endian
    this.offset += 2;
    return this;
  }

  writeUint32(value) {
    this._ensureCapacity(4);
    this.view.setUint32(this.offset, value, false); // big-endian
    this.offset += 4;
    return this;
  }

  writeUTF8String(str) {
    const encoded = encoder.encode(str);
    this._ensureCapacity(encoded.length);
    this.bytes.set(encoded, this.offset);
    this.offset += encoded.length;
    return this;
  }

  toBinary() {
    // Return binary string (compatible with existing code)
    const bytes = new Uint8Array(this.buffer, 0, this.offset);
    return String.fromCharCode.apply(null, bytes);
  }

  reset() {
    this.offset = 0;
    return this;
  }

  _ensureCapacity(additional) {
    const needed = this.offset + additional;
    if (needed <= this.buffer.byteLength) return;

    // Grow buffer (double until big enough)
    let newCapacity = this.buffer.byteLength * 2;
    while (newCapacity < needed) {
      newCapacity *= 2;
    }

    const newBuffer = new ArrayBuffer(newCapacity);
    const newBytes = new Uint8Array(newBuffer);
    newBytes.set(this.bytes);

    this.buffer = newBuffer;
    this.view = new DataView(newBuffer);
    this.bytes = newBytes;
  }
}

// ============================================================================
// Binary Packet Parser (replaces ByteBuffer.fromBinary())
// ============================================================================

export class BinaryParser {
  constructor(binaryString) {
    // Convert binary string to Uint8Array
    const length = binaryString.length;
    this.bytes = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      this.bytes[i] = binaryString.charCodeAt(i) & 0xff;
    }
    this.view = new DataView(this.bytes.buffer);
    this.offset = 0;
    this.length = length;
  }

  readUint8() {
    if (this.offset + 1 > this.length) {
      throw new RangeError(`BinaryParser: Cannot read Uint8 at offset ${this.offset}`);
    }
    const value = this.view.getUint8(this.offset);
    this.offset += 1;
    return value;
  }

  readUint16() {
    if (this.offset + 2 > this.length) {
      throw new RangeError(`BinaryParser: Cannot read Uint16 at offset ${this.offset}`);
    }
    const value = this.view.getUint16(this.offset, false); // big-endian
    this.offset += 2;
    return value;
  }

  readUint32() {
    if (this.offset + 4 > this.length) {
      throw new RangeError(`BinaryParser: Cannot read Uint32 at offset ${this.offset}`);
    }
    const value = this.view.getUint32(this.offset, false); // big-endian
    this.offset += 4;
    return value;
  }

  readBytes(length) {
    if (this.offset + length > this.length) {
      throw new RangeError(`BinaryParser: Cannot read ${length} bytes at offset ${this.offset}`);
    }
    const slice = this.bytes.slice(this.offset, this.offset + length);
    this.offset += length;

    // Return object with toUTF8() method to match ByteBuffer API
    return {
      toUTF8: () => decoder.decode(slice),
      toString: (encoding) => {
        if (encoding === 'binary') {
          return String.fromCharCode.apply(null, slice);
        }
        return decoder.decode(slice);
      }
    };
  }

  toUTF8() {
    return decoder.decode(this.bytes);
  }

  toString(encoding) {
    if (encoding === 'binary') {
      return String.fromCharCode.apply(null, this.bytes);
    }
    return decoder.decode(this.bytes);
  }
}
