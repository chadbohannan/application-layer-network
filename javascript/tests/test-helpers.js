/**
 * Test helpers and mocks for browser APIs
 */

// Mock WebSocket for Node.js environment
export class MockWebSocket {
  constructor(url) {
    this.url = url;
    this.readyState = 0; // CONNECTING
    this.onopen = null;
    this.onclose = null;
    this.onerror = null;
    this.onmessage = null;
    this._messageQueue = [];
  }

  // Simulate connection opening
  _triggerOpen() {
    this.readyState = 1; // OPEN
    if (this.onopen) {
      this.onopen({ type: 'open' });
    }
  }

  // Simulate receiving a message
  _receiveMessage(data) {
    if (this.onmessage) {
      this.onmessage({ data, type: 'message' });
    }
  }

  // Simulate connection closing
  _triggerClose() {
    this.readyState = 3; // CLOSED
    if (this.onclose) {
      this.onclose({ type: 'close' });
    }
  }

  send(data) {
    if (this.readyState !== 1) {
      throw new Error('WebSocket is not open');
    }
    this._messageQueue.push(data);
  }

  close() {
    this._triggerClose();
  }

  // Test helper to get sent messages
  _getSentMessages() {
    return [...this._messageQueue];
  }

  _clearMessages() {
    this._messageQueue = [];
  }
}

// Mock Channel for testing
export class MockChannel {
  constructor() {
    this.sentPackets = [];
    this.onPacket = null;
    this.onClose = null;
  }

  send(packet) {
    this.sentPackets.push(packet);
  }

  close() {
    if (this.onClose) {
      this.onClose();
    }
  }

  // Test helper to simulate receiving a packet
  _receivePacket(packet) {
    if (this.onPacket) {
      this.onPacket(packet);
    }
  }

  // Test helper to get sent packets
  _getSentPackets() {
    return [...this.sentPackets];
  }

  _clearSentPackets() {
    this.sentPackets = [];
  }
}
