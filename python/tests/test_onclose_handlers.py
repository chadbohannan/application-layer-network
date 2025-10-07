import pytest
import os
import selectors
import sys
import socket
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..' , 'src')))
from alnmeshpy import LocalChannel, TcpChannel
from threading import Event
import time

# Try to import optional channel types that may not be available
try:
    from alnmeshpy import BLEChannel
    HAS_BLE = True
except ImportError:
    HAS_BLE = False

try:
    from alnmeshpy import BtChannel
    HAS_BT = True
except ImportError:
    HAS_BT = False


class TestOnCloseHandlers:
    """Test OnClose handler functionality for all channel types"""

    def test_local_channel_single_onclose_handler(self):
        """Test LocalChannel with single OnClose handler"""
        r, w = os.pipe()
        channel = LocalChannel(r, w)

        close_called = Event()
        received_channel = []

        def on_close_handler(ch):
            received_channel.append(ch)
            close_called.set()

        channel.on_close(on_close_handler)

        # Setup selector like the router does
        sel = selectors.DefaultSelector()
        try:
            channel.listen(sel, lambda ch, pkt: None)
            channel.close()

            assert close_called.wait(timeout=1), "OnClose handler was not called"
            assert len(received_channel) == 1, "OnClose handler called wrong number of times"
            assert received_channel[0] is channel, "OnClose handler received wrong channel"
        finally:
            sel.close()

    def test_local_channel_multiple_onclose_handlers(self):
        """Test LocalChannel with multiple OnClose handlers"""
        r, w = os.pipe()
        channel = LocalChannel(r, w)

        handler1_called = Event()
        handler2_called = Event()
        handler3_called = Event()
        received_channels = []

        def handler1(ch):
            received_channels.append(('handler1', ch))
            handler1_called.set()

        def handler2(ch):
            received_channels.append(('handler2', ch))
            handler2_called.set()

        def handler3(ch):
            received_channels.append(('handler3', ch))
            handler3_called.set()

        # Add handlers one by one
        channel.on_close(handler1)
        channel.on_close(handler2)
        channel.on_close(handler3)

        # Setup selector like the router does
        sel = selectors.DefaultSelector()
        try:
            channel.listen(sel, lambda ch, pkt: None)
            channel.close()

            assert handler1_called.wait(timeout=1), "Handler1 was not called"
            assert handler2_called.wait(timeout=1), "Handler2 was not called"
            assert handler3_called.wait(timeout=1), "Handler3 was not called"
            assert len(received_channels) == 3, "Wrong number of handler calls"

            # Verify all handlers received the same channel instance
            for handler_name, ch in received_channels:
                assert ch is channel, f"{handler_name} received wrong channel"
        finally:
            sel.close()

    def test_local_channel_multiple_handlers_at_once(self):
        """Test LocalChannel with multiple OnClose handlers added in one call"""
        r, w = os.pipe()
        channel = LocalChannel(r, w)

        handler1_called = Event()
        handler2_called = Event()
        received_channels = []

        def handler1(ch):
            received_channels.append(('handler1', ch))
            handler1_called.set()

        def handler2(ch):
            received_channels.append(('handler2', ch))
            handler2_called.set()

        # Add multiple handlers in single call (like Go implementation)
        channel.on_close(handler1, handler2)

        # Setup selector like the router does
        sel = selectors.DefaultSelector()
        try:
            channel.listen(sel, lambda ch, pkt: None)
            channel.close()

            assert handler1_called.wait(timeout=1), "Handler1 was not called"
            assert handler2_called.wait(timeout=1), "Handler2 was not called"
            assert len(received_channels) == 2, "Wrong number of handler calls"
        finally:
            sel.close()

    def test_tcp_channel_onclose_handler(self):
        """Test TcpChannel OnClose handler"""
        # Create a connected socket pair for testing
        server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_sock.bind(('localhost', 0))
        server_sock.listen(1)

        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect(server_sock.getsockname())

        conn_sock, _ = server_sock.accept()

        channel = TcpChannel(conn_sock)

        close_called = Event()
        received_channel = []

        def on_close_handler(ch):
            received_channel.append(ch)
            close_called.set()

        channel.on_close(on_close_handler)
        channel.close()

        assert close_called.wait(timeout=1), "OnClose handler was not called"
        assert len(received_channel) == 1, "OnClose handler called wrong number of times"
        assert received_channel[0] is channel, "OnClose handler received wrong channel"

        # Cleanup
        client_sock.close()
        server_sock.close()

    @pytest.mark.skipif(not HAS_BT, reason="BtChannel not available")
    def test_bt_channel_onclose_handler(self):
        """Test BtChannel OnClose handler (after fix)"""
        # Create a connected socket pair for testing
        server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_sock.bind(('localhost', 0))
        server_sock.listen(1)

        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect(server_sock.getsockname())

        conn_sock, _ = server_sock.accept()

        channel = BtChannel(conn_sock)

        close_called = Event()
        received_channel = []

        def on_close_handler(ch):
            received_channel.append(ch)
            close_called.set()

        channel.on_close(on_close_handler)

        # Setup selector for BtChannel
        sel = selectors.DefaultSelector()
        try:
            channel.listen(sel, lambda ch, pkt: None)
            channel.close()

            assert close_called.wait(timeout=1), "OnClose handler was not called"
            assert len(received_channel) == 1, "OnClose handler called wrong number of times"
            assert received_channel[0] is channel, "OnClose handler received wrong channel"
        finally:
            sel.close()
            client_sock.close()
            server_sock.close()

    @pytest.mark.skipif(not HAS_BT, reason="BtChannel not available")
    def test_bt_channel_multiple_onclose_handlers(self):
        """Test BtChannel with multiple OnClose handlers (testing the fix)"""
        # Create a connected socket pair for testing
        server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_sock.bind(('localhost', 0))
        server_sock.listen(1)

        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect(server_sock.getsockname())

        conn_sock, _ = server_sock.accept()

        channel = BtChannel(conn_sock)

        handler1_called = Event()
        handler2_called = Event()
        received_channels = []

        def handler1(ch):
            received_channels.append(('handler1', ch))
            handler1_called.set()

        def handler2(ch):
            received_channels.append(('handler2', ch))
            handler2_called.set()

        channel.on_close(handler1)
        channel.on_close(handler2)

        # Setup selector for BtChannel
        sel = selectors.DefaultSelector()
        try:
            channel.listen(sel, lambda ch, pkt: None)
            channel.close()

            assert handler1_called.wait(timeout=1), "Handler1 was not called"
            assert handler2_called.wait(timeout=1), "Handler2 was not called"
            assert len(received_channels) == 2, "Wrong number of handler calls"

            # Verify all handlers received the same channel instance
            for handler_name, ch in received_channels:
                assert ch is channel, f"{handler_name} received wrong channel"
        finally:
            sel.close()
            client_sock.close()
            server_sock.close()

    def test_onclose_handler_exception_handling(self):
        """Test that exceptions in OnClose handlers don't break other handlers"""
        r, w = os.pipe()
        channel = LocalChannel(r, w)

        handler1_called = Event()
        handler2_called = Event()
        handler3_called = Event()

        def failing_handler(ch):
            handler1_called.set()
            raise Exception("Handler intentionally failed")

        def working_handler2(ch):
            handler2_called.set()

        def working_handler3(ch):
            handler3_called.set()

        channel.on_close(failing_handler)
        channel.on_close(working_handler2)
        channel.on_close(working_handler3)

        # Setup selector like the router does
        sel = selectors.DefaultSelector()
        try:
            channel.listen(sel, lambda ch, pkt: None)
            channel.close()

            # Even with one handler failing, other handlers should still be called
            assert handler1_called.wait(timeout=1), "Failing handler was not called"
            assert handler2_called.wait(timeout=1), "Handler2 was not called after exception"
            assert handler3_called.wait(timeout=1), "Handler3 was not called after exception"
        finally:
            sel.close()

    def test_onclose_no_handlers_registered(self):
        """Test that close works properly when no OnClose handlers are registered"""
        r, w = os.pipe()
        channel = LocalChannel(r, w)

        # Setup selector like the router does
        sel = selectors.DefaultSelector()
        try:
            channel.listen(sel, lambda ch, pkt: None)
            # Should not raise any exceptions
            channel.close()

            # File descriptors should be closed (will raise exception if we try to read)
            with pytest.raises(OSError):
                os.read(r, 1)
        finally:
            sel.close()