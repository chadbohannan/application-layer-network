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
    this.onPacket = () => {}
  }

  send (packet) {
    this.twin.onPacket(packet)
  }
}

describe('# Test Router', function () {
  it('# route to a local service', async function () {
    let recievedPacket = null
    const router = new Router('42')
    router.registerService(1, (p) => { recievedPacket = p })

    const localChannel = new LocalChannel()
    router.addChannel(localChannel)

    const packet = new Packet()
    packet.srv = 1
    localChannel.onPacket(packet)
    assert.isNotNull(recievedPacket)
  })

  it('# route to a neighboring service (1 hop)', async function () {
    // TODO create Routers router1 and router2
    // TODO router1.onPacket(packet)
    // TODO assert router2 service was called
  })

  it('# route to a non-neighboring service (2 hops)', async function () {
    // TODO create Routers router1 and router2
    // TODO router1.onPacket(packet)
    // TODO assert router2 service was called
  })
})
