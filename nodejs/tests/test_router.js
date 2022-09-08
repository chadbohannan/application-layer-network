// const ByteBuffer = require('bytebuffer')
// const { Packet } = require('../aln/packet')
// const { Parser } = require('../aln/parser')
const chai = require('chai')
const assert = chai.assert
const { describe, it } = require('mocha')
const { Packet } = require('../aln/packet')
const Router = require('../aln/router')

class LocalChannel {
  constructor (twin) {
    this.twin = twin || new LocalChannel(this)
    this.onPacket = (packet) => {}
  }

  send (packet) {
    setTimeout(() => this.twin.onPacket(packet))
  }
}

describe('# Test Router', function () {
  it('# route to a local service', async function () {
    let recievedPacket = null
    const router = new Router('42')
    router.registerService('test', (p) => { recievedPacket = p })

    const localChannel = new LocalChannel()
    router.addChannel(localChannel)

    const packet = new Packet()
    packet.srv = 'test'
    localChannel.onPacket(packet)
    assert.isNotNull(recievedPacket)
  })

  it('# route to a neighboring service (1 hop)', async function () {
    const r1 = new Router('1')
    const r2 = new Router('2')
    let recievedPacket = null
    r2.registerService('test', (packet) => { recievedPacket = packet })

    const ch = new LocalChannel()
    r1.addChannel(ch)
    r2.addChannel(ch.twin)
    await new Promise(resolve => setTimeout(resolve, 10))

    const p = new Packet()
    p.srv = 'test'
    r1.send(p)
    await new Promise(resolve => setTimeout(resolve, 1))

    assert.isNotNull(recievedPacket)
  })

  it('# route to a non-neighboring service (2 hops)', async function () {
    const r1 = new Router('1')
    const r2 = new Router('2')
    const r3 = new Router('3')

    let recievedPacket = null
    r3.registerService('test', (packet) => { recievedPacket = packet })

    const ch1 = new LocalChannel()
    r1.addChannel(ch1)
    r2.addChannel(ch1.twin)
    await new Promise(resolve => setTimeout(resolve, 10))

    const ch2 = new LocalChannel()
    r2.addChannel(ch2)
    r3.addChannel(ch2.twin)
    await new Promise(resolve => setTimeout(resolve, 10))

    const p = new Packet()
    p.srv = 'test'
    r1.send(p)
    await new Promise(resolve => setTimeout(resolve, 10))

    assert.isNotNull(recievedPacket)
  })
})
