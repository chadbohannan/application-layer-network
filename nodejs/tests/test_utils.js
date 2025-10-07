// const ByteBuffer = require('bytebuffer')
// const { Packet } = require('../lib/aln/packet')
const {
  makeNetQueryPacket,
  makeNetworkRouteSharePacket,
  parseNetworkRouteSharePacket,
  makeNetworkServiceSharePacket,
  parseNetworkServiceSharePacket
} = require('../lib/aln/utils')
const chai = require('chai')
const assert = chai.assert
const { describe, it } = require('mocha')

describe('# Test Utils', function () {
  it('# makes a NET_QUERY packet', async function () {
    const p = makeNetQueryPacket()
    assert.equal(p.net, 3)
    assert.equal(p.toBinary().length, 3)
  })

  it('# makes a NET_ROUTE packet', async function () {
    const p = makeNetworkRouteSharePacket('42', '8675309', 2)
    assert.equal(p.toBinary().length, 18)
    const [dest, nextHop, cost, err] = parseNetworkRouteSharePacket(p)
    assert.isNull(err)
    assert.equal(dest, '8675309')
    assert.equal(nextHop, '42')
    assert.equal(cost, 2)
  })

  it('# makes a NET_SERVICE packet', async function () {
    const p = makeNetworkServiceSharePacket('42', 'test', 2)
    assert.equal(p.toBinary().length, 15)
    const [address, serviceID, load, err] = parseNetworkServiceSharePacket(p)
    assert.isNull(err)
    assert.equal(address, '42')
    assert.equal(serviceID, 'test')
    assert.equal(load, 2)
  })
})
