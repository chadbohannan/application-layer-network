import pytest
import os
import selectors
import sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..' , 'src')))
from alnmeshpy import LocalChannel, Router, Packet
from threading import Event

# Fixture for setting up routers and channels
@pytest.fixture
def three_routers_setup():
    # Setup for Router 1 and its channels
    r1a, w1a = os.pipe()
    r2a, w2a = os.pipe()
    ch1a = LocalChannel(r1a, w2a)
    ch2a = LocalChannel(r2a, w1a)

    sel = selectors.DefaultSelector()

    # Configure the ping handler on node 1
    received_ping = Event()
    ping_response_sent = Event()

    def ping_handler(packet):
        received_ping.set()
        router1.send(Packet(
            destAddr=packet.srcAddr,
            contextID=packet.contextID,
            data="pong"
        ))
        ping_response_sent.set()

    router1 = Router(sel, "aln-node-1")
    router1.start()
    router1.register_service("ping", ping_handler)
    router1.add_channel(ch1a)

    # Setup for Router 2
    router2 = Router(sel, "aln-node-2")
    router2.start()
    router2.add_channel(ch2a)

    # Setup for Router 3 and its channels
    r1b, w1b = os.pipe()
    r2b, w2b = os.pipe()
    ch1b = LocalChannel(r1b, w2b)
    ch2b = LocalChannel(r2b, w1b)

    received_pong = Event()
    pong_data = {"data": None}  # Use dict so changes are reflected

    def on_packet(packet):
        pong_data["data"] = packet.data
        received_pong.set()

    router3 = Router(sel, "aln-node-3")
    router3.start()
    ctxID = router3.register_context_handler(on_packet)

    # Flag to control when to send ping
    ping_service_available = Event()
    ping_service_address = {"addr": None}

    def on_service_discovery(service, load, address):
        if service == "ping":
            ping_service_address["addr"] = address
            ping_service_available.set()

    router3.set_on_service_load_changed_handler(on_service_discovery)
    router3.add_channel(ch2b)
    router2.add_channel(ch1b)

    yield router1, router2, router3, sel, received_ping, ping_response_sent, received_pong, pong_data, ctxID, ping_service_available

    # Teardown
    router1.stop()
    router2.stop()
    router3.stop()
    ch1a.close()
    ch2a.close()
    ch1b.close()
    ch2b.close()
    sel.close()
    # File descriptors are already closed by LocalChannel.close() calls above


def test_three_routers_ping_pong(three_routers_setup):
    router1, router2, router3, sel, received_ping, ping_response_sent, received_pong, pong_data, ctxID, ping_service_available = three_routers_setup

    # Give some time for network discovery to complete
    import time
    time.sleep(0.5)

    # Manually trigger network state sharing to ensure service discovery
    router1.share_net_state()
    router2.share_net_state()
    router3.share_net_state()

    # Give more time for route discovery to propagate through all hops
    time.sleep(2.0)

    # Trigger again to ensure route propagation
    router1.share_net_state()
    router2.share_net_state()
    router3.share_net_state()

    time.sleep(1.0)

    # Wait for service discovery to complete
    assert ping_service_available.wait(timeout=10), "Ping service was not discovered"

    # Now send the ping manually from the test
    text = "hello world".encode(encoding="utf-8")
    router3.send(Packet(service="ping", contextID=ctxID, data=text))

    # Wait for the ping to be received by router1
    assert received_ping.wait(timeout=10), "Router 1 did not receive ping"
    # Wait for router1 to send the pong response
    assert ping_response_sent.wait(timeout=10), "Router 1 did not send pong response"
    # Wait for router3 to receive the pong response
    assert received_pong.wait(timeout=10), "Router 3 did not receive pong"

    assert pong_data["data"] is not None, "No pong data received"
    assert pong_data["data"].decode(encoding="utf-8") == "pong", f"Expected 'pong', got {pong_data['data'].decode(encoding='utf-8')}"
