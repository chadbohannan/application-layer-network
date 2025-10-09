// const ByteBuffer = require('bytebuffer')
const { Packet } = require('../lib/aln/packet')
const { Parser, toAx25Frame } = require('../lib/aln/parser')
const chai = require('chai')
const assert = chai.assert
const { describe, it } = require('mocha')

describe('# Test Packet', function () {
  it('# serializes and parses', async function () {
    let parsedPacket = null
    const parser = new Parser((packet) => { parsedPacket = packet })
    const packet = new Packet()
    packet.srv = 'ping'
    packet.src = '1'
    packet.dst = '42'
    packet.nxt = '8675309'
    const pBuf = packet.toBinary()

    parsedPacket = new Packet(pBuf)
    assert.equal(parsedPacket.toJson(), packet.toJson())
    parsedPacket = null

    const frame = toAx25Frame(pBuf)
    parser.readAx25FrameBytes(frame, frame.length)
    assert.isNotNull(parsedPacket)
    assert.equal(parsedPacket.toJson(), packet.toJson())
  })
})
