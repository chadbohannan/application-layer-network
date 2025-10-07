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

describe('# Three Router Topology Tests', function () {
  this.timeout(15000) // Increase timeout for network discovery

  it('# ping-pong across three routers (multi-hop)', async function () {
    // Create three routers in a chain topology:
    // Router1 <---> Router2 <---> Router3
    //
    // Router1 has the "ping" service
    // Router3 will discover and send a ping request
    // Router2 acts as a relay node to create a multihop nework

    // Setup Router 1 and Router 2 connection
    const ch1a = new LocalChannel()
    const ch2a = ch1a.twin

    // Create Router 1 with ping service
    const router1 = new Router('aln-node-1')
    router1.registerService('ping', (packet) => {
      console.log('Router 1: Received ping request')

      // Send pong response back to the source
      const pongPacket = new Packet()
      pongPacket.dst = packet.src
      pongPacket.ctx = packet.ctx
      pongPacket.data = 'pong'
      router1.send(pongPacket)
      console.log('Router 1: Sent pong response')
    })
    router1.addChannel(ch1a)

    // Create Router 2 (relay)
    const router2 = new Router('aln-node-2')
    router2.addChannel(ch2a)

    // Setup Router 2 and Router 3 connection
    const ch1b = new LocalChannel()
    const ch2b = ch1b.twin

    // Create Router 3 (client)
    const router3 = new Router('aln-node-3')

    // Register context handler for pong response
    let receivedPong = false
    const ctxID = router3.registerContextHandler((packet) => {
      console.log('Router 3: Received pong response', packet.data)
      receivedPong = true
    })

    router3.setOnServiceCapacityChanged((service, capacity, address) => {
      console.log(`Router 3: Service discovered - ${service} at ${address} (capacity: ${capacity})`)
      if (service === 'ping' && capacity > 0) {
        // Send ping request from Router 3
        const pingPacket = new Packet()
        pingPacket.srv = 'ping'
        pingPacket.ctx = ctxID
        pingPacket.data = 'hello world'
        router3.send(pingPacket)
      }
    })

    // Connect Router 3 and Router 2
    router3.addChannel(ch2b)
    router2.addChannel(ch1b)

    console.log('All routers connected, waiting for network discovery...')

    await waitForEvent(() => receivedPong, 5000) // 5 second wait for test to pass
    assert.isTrue(receivedPong, 'Pong Recieved')
    console.log('✅ Three-router ping-pong test passed!')
  })

  it('# service multicast across three routers', async function () {
    // Create three routers where Router1 and Router3 both have the same service
    // Router2 acts as a relay node; represents any number of nodes in a multihop network
    // Router3 sends a multicast that should reach both local and remote echo services

    // Setup Router 1 with echo service
    const ch1a = new LocalChannel()
    const ch2a = ch1a.twin

    let router1Received = false
    const router1 = new Router('aln-node-1')
    router1.registerService('echo', (packet) => {
      console.log('Router 1: Received echo request')
      router1Received = true
    })
    router1.addChannel(ch1a)

    // Setup Router 2 as passive relay
    const router2 = new Router('aln-node-2')
    router2.addChannel(ch2a)

    // Setup Router 3 with echo service
    const ch1b = new LocalChannel()
    const ch2b = ch1b.twin

    let router3LocalReceived = false
    const router3 = new Router('aln-node-3')
    router3.registerService('echo', (packet) => {
      console.log('Router 3: Received echo request (local)')
      router3LocalReceived = true
    })

    // Track when Router 3 discovers Router 1's echo service
    let router1ServiceDiscovered = false
    router3.setOnServiceCapacityChanged((service, capacity, address) => {
      if (service === 'echo' && capacity > 0 && address === 'aln-node-1') {
        console.log(`Router 3: Discovered echo service at ${address}`)
        router1ServiceDiscovered = true
      }
    })

    router3.addChannel(ch2b)
    router2.addChannel(ch1b)

    console.log('All routers connected, waiting for network discovery...')

    // Wait for Router 3 to discover Router 1's echo service
    await waitForEvent(() => router1ServiceDiscovered, 5000)
    console.log('Router 3 has discovered Router 1 echo service')

    // Send service multicast from Router 3 (should reach both local and remote)
    const echoPacket = new Packet()
    echoPacket.srv = 'echo'
    echoPacket.data = 'broadcast message'
    router3.send(echoPacket)

    // Wait for both services to receive the message
    console.log('Waiting for multicast delivery...')
    await waitForEvent(() => router1Received && router3LocalReceived, 5000)

    assert.isTrue(router1Received, 'Router 1 should receive echo')
    assert.isTrue(router3LocalReceived, 'Router 3 should receive echo locally')

    console.log('✅ Service multicast test passed!')
  })
})
