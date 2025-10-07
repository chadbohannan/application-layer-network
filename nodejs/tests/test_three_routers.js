const chai = require('chai')
const assert = chai.assert
const { describe, it } = require('mocha')
const { Packet } = require('../lib/aln/packet')
const Router = require('../lib/aln/router')

// LocalChannel implementation - simulates a bidirectional communication channel
// Each channel has a twin, and sends packets to its twin with a small async delay
class LocalChannel {
  constructor (twin) {
    this.twin = twin || new LocalChannel(this)
    this.onPacket = (packet) => {}
    this.onClose = null
  }

  send (packet) {
    // Simulate async packet delivery
    setTimeout(() => {
      if (this.twin && this.twin.onPacket) {
        this.twin.onPacket(packet)
      }
    }, 5)
  }

  close () {
    if (this.onClose) {
      this.onClose()
    }
  }
}

// Helper to wait for an event with timeout
function waitForEvent (checkFn, timeoutMs = 5000, checkIntervalMs = 50) {
  return new Promise((resolve, reject) => {
    const startTime = Date.now()
    const interval = setInterval(() => {
      if (checkFn()) {
        clearInterval(interval)
        resolve(true)
      } else if (Date.now() - startTime > timeoutMs) {
        clearInterval(interval)
        reject(new Error('Timeout waiting for event'))
      }
    }, checkIntervalMs)
  })
}

// Helper to wait for a specified time
function wait (ms) {
  return new Promise(resolve => setTimeout(resolve, ms))
}

describe('# Three Router Topology Tests', function () {
  this.timeout(15000) // Increase timeout for network discovery

  it('# ping-pong across three routers (multi-hop)', async function () {
    // Create three routers in a chain topology:
    // Router1 <---> Router2 <---> Router3
    //
    // Router1 has the "ping" service
    // Router3 will discover and send a ping request
    // Router2 acts as a relay between them

    // Setup Router 1 and Router 2 connection
    const ch1a = new LocalChannel()
    const ch2a = ch1a.twin

    // Track ping handler execution
    let receivedPing = false
    let pingResponseSent = false
    let receivedPingData = null

    // Create Router 1 with ping service
    const router1 = new Router('aln-node-1')
    router1.registerService('ping', (packet) => {
      console.log('Router 1: Received ping request')
      receivedPing = true
      receivedPingData = packet.data

      // Send pong response back to the source
      const pongPacket = new Packet()
      pongPacket.dst = packet.src
      pongPacket.ctx = packet.ctx
      pongPacket.data = 'pong'
      router1.send(pongPacket)
      pingResponseSent = true
      console.log('Router 1: Sent pong response')
    })
    router1.addChannel(ch1a)

    // Create Router 2 (relay)
    const router2 = new Router('aln-node-2')
    router2.addChannel(ch2a)

    // Setup Router 2 and Router 3 connection
    const ch1b = new LocalChannel()
    const ch2b = ch1b.twin

    // Track pong response reception
    let receivedPong = false
    let pongData = null

    // Create Router 3 (client)
    const router3 = new Router('aln-node-3')

    // Register context handler for pong response
    const ctxID = router3.registerContextHandler((packet) => {
      console.log('Router 3: Received pong response')
      pongData = packet.data
      receivedPong = true
    })

    // Track service discovery
    let pingServiceDiscovered = false
    let pingServiceAddress = null

    router3.setOnServiceCapacityChanged((service, capacity, address) => {
      console.log(`Router 3: Service discovered - ${service} at ${address} (capacity: ${capacity})`)
      if (service === 'ping' && capacity > 0) {
        pingServiceAddress = address
        pingServiceDiscovered = true
      }
    })

    // Connect Router 3 and Router 2
    router3.addChannel(ch2b)
    router2.addChannel(ch1b)

    console.log('All routers connected, waiting for network discovery...')

    // Wait for initial network discovery
    await wait(500)

    // Manually trigger network state sharing to ensure discovery
    router1.shareNetState()
    router2.shareNetState()
    router3.shareNetState()

    // Wait for route propagation through all hops
    await wait(1000)

    // Trigger again to ensure full propagation
    router1.shareNetState()
    router2.shareNetState()
    router3.shareNetState()

    await wait(500)

    // Wait for service discovery with timeout
    console.log('Waiting for ping service discovery...')
    await waitForEvent(() => pingServiceDiscovered, 10000)
    assert.isTrue(pingServiceDiscovered, 'Ping service should be discovered')
    assert.equal(pingServiceAddress, 'aln-node-1', 'Ping service should be at aln-node-1')

    console.log('Ping service discovered! Sending ping request...')

    // Send ping request from Router 3
    const pingPacket = new Packet()
    pingPacket.srv = 'ping'
    pingPacket.ctx = ctxID
    pingPacket.data = 'hello world'
    router3.send(pingPacket)

    // Wait for ping to be received by Router 1
    console.log('Waiting for Router 1 to receive ping...')
    await waitForEvent(() => receivedPing, 5000)
    assert.isTrue(receivedPing, 'Router 1 should receive ping')
    assert.equal(receivedPingData, 'hello world', 'Ping data should match')

    // Wait for Router 1 to send pong response
    console.log('Waiting for Router 1 to send pong...')
    await waitForEvent(() => pingResponseSent, 5000)
    assert.isTrue(pingResponseSent, 'Router 1 should send pong response')

    // Wait for Router 3 to receive pong response
    console.log('Waiting for Router 3 to receive pong...')
    await waitForEvent(() => receivedPong, 5000)
    assert.isTrue(receivedPong, 'Router 3 should receive pong')
    assert.equal(pongData, 'pong', 'Pong data should be "pong"')

    console.log('✅ Three-router ping-pong test passed!')
  })

  it('# service multicast across three routers', async function () {
    // Create three routers where Router1 and Router3 both have the same service
    // Router2 in the middle should route to both

    // Setup Router 1
    const ch1a = new LocalChannel()
    const ch2a = ch1a.twin

    let router1Received = false
    const router1 = new Router('aln-node-1')
    router1.registerService('echo', (packet) => {
      console.log('Router 1: Received echo request')
      router1Received = true
    })
    router1.addChannel(ch1a)

    // Setup Router 2 (relay)
    const router2 = new Router('aln-node-2')
    router2.addChannel(ch2a)

    // Setup Router 3
    const ch1b = new LocalChannel()
    const ch2b = ch1b.twin

    let router3Received = false
    const router3 = new Router('aln-node-3')
    router3.registerService('echo', (packet) => {
      console.log('Router 3: Received echo request')
      router3Received = true
    })
    router3.addChannel(ch2b)
    router2.addChannel(ch1b)

    console.log('All routers connected, waiting for network discovery...')

    // Wait for network discovery
    await wait(500)
    router1.shareNetState()
    router2.shareNetState()
    router3.shareNetState()
    await wait(1000)
    router1.shareNetState()
    router2.shareNetState()
    router3.shareNetState()
    await wait(500)

    console.log('Sending multicast echo request from Router 2...')

    // Send service multicast from Router 2 (should reach both Router 1 and Router 3)
    const echoPacket = new Packet()
    echoPacket.srv = 'echo'
    echoPacket.data = 'broadcast message'
    router2.send(echoPacket)

    // Wait for both routers to receive the message
    console.log('Waiting for multicast delivery...')
    await waitForEvent(() => router1Received && router3Received, 5000)

    assert.isTrue(router1Received, 'Router 1 should receive echo')
    assert.isTrue(router3Received, 'Router 3 should receive echo')

    console.log('✅ Service multicast test passed!')
  })
})
