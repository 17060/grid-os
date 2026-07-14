"""GridBasic IRC module — a real, socket-based IRC client.

Usage from GridBasic::

    import irc
    conn = IRC.connect("irc.libera.chat", 6667, "GridBot")
    conn.join("#gridbasic")
    conn.on_message(fn(channel, sender, text) ->
        if text == "!hello" then conn.send(channel, "hi " + sender))
    conn.loop()

The connection runs the IRC protocol over a plain TCP socket (TLS supported
when ``use_tls`` is true). Incoming PRIVMSGs are dispatched to the registered
callback as native GridBasic calls.
"""

from __future__ import annotations

import socket
import ssl
import threading
import time as _time

from ..interpreter import BoundBuiltin
from ..errors import GBRuntimeError


class IRCConnection:
    def __init__(self, host, port, nick, use_tls=False, interp=None):
        self.host = host
        self.port = int(port)
        self.nick = nick
        self.use_tls = use_tls
        self.sock = None
        self.reader = None
        self._on_message = None
        self._on_join = None
        self._on_raw = None
        self._stop = False
        self._lock = threading.Lock()
        self._channels = []
        self.interp = interp
        self._buffer = b""

    def _connect(self):
        try:
            raw = socket.create_connection((self.host, self.port), timeout=15)
        except Exception as e:
            raise GBRuntimeError(f"IRC connection failed: {e}")
        if self.use_tls:
            ctx = ssl.create_default_context()
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE
            raw = ctx.wrap_socket(raw, server_hostname=self.host)
        self.sock = raw
        self.sock.settimeout(None)
        self._send_raw(f"NICK {self.nick}\r\n")
        self._send_raw(f"USER {self.nick} 0 * :GridBasic IRC\r\n")

    def _send_raw(self, line):
        if self.sock is None:
            raise GBRuntimeError("IRC: not connected")
        with self._lock:
            try:
                self.sock.sendall((line + "\r\n").encode("utf-8", "replace"))
            except Exception as e:
                raise GBRuntimeError(f"IRC send error: {e}")

    def _read_lines(self):
        while not self._stop:
            try:
                data = self.sock.recv(4096)
            except OSError:
                break
            if not data:
                break
            self._buffer += data
            while b"\r\n" in self._buffer:
                line, self._buffer = self._buffer.split(b"\r\n", 1)
                self._dispatch(line.decode("utf-8", "replace"))

    def _dispatch(self, line):
        if self._on_raw is not None and self.interp is not None:
            try: self.interp.call_value(self._on_raw, [line], {})
            except Exception: pass
        if not line:
            return
        if line.startswith("PING"):
            self._send_raw("PONG " + line.split(" ", 1)[1])
            return
        # parse prefix and command
        prefix = ""
        if line.startswith(":"):
            prefix, line = line[1:].split(" ", 1)
        parts = line.split(" ", 2)
        cmd = parts[0]
        if cmd == "PRIVMSG" and len(parts) >= 3:
            target = parts[1]
            msg = parts[2]
            if msg.startswith(":"):
                msg = msg[1:]
            sender = prefix.split("!", 1)[0]
            if self._on_message is not None and self.interp is not None:
                try:
                    self.interp.call_value(self._on_message, [target, sender, msg], {})
                except Exception as e:
                    if self.interp:
                        self.interp.emit(f"IRC callback error: {e}\n")
            return
        if cmd == "JOIN" and len(parts) >= 2:
            channel = parts[1]
            if channel.startswith(":"):
                channel = channel[1:]
            sender = prefix.split("!", 1)[0]
            if self._on_join is not None and self.interp is not None:
                try:
                    self.interp.call_value(self._on_join, [channel, sender], {})
                except Exception:
                    pass

    # ---- public API (called from GridBasic) ---------------------------
    def join(self, channel):
        if not channel.startswith("#"):
            channel = "#" + channel
        self._send_raw(f"JOIN {channel}")
        if channel not in self._channels:
            self._channels.append(channel)
        return None

    def part(self, channel):
        if not channel.startswith("#"):
            channel = "#" + channel
        self._send_raw(f"PART {channel}")
        if channel in self._channels:
            self._channels.remove(channel)
        return None

    def send(self, target, msg):
        self._send_raw(f"PRIVMSG {target} :{msg}")
        return None

    def notice(self, target, msg):
        self._send_raw(f"NOTICE {target} :{msg}")
        return None

    def action(self, target, msg):
        self._send_raw(f"PRIVMSG {target} :\x01ACTION {msg}\x01")
        return None

    def nick(self, newnick):
        self._send_raw(f"NICK {newnick}")
        self.nick = newnick
        return None

    def quit(self, msg="bye"):
        try: self._send_raw(f"QUIT :{msg}")
        except Exception: pass
        self._stop = True
        try: self.sock.close()
        except Exception: pass
        return None

    def on_message(self, fn):
        self._on_message = fn
        return None

    def on_join(self, fn):
        self._on_join = fn
        return None

    def on_raw(self, fn):
        self._on_raw = fn
        return None

    def loop(self, timeout=None):
        if self.sock is None:
            self._connect()
        if self.reader is None or not self.reader.is_alive():
            self.reader = threading.Thread(target=self._read_lines, daemon=True)
            self.reader.start()
        # block until stopped or timeout
        start = _time.time()
        while not self._stop:
            _time.sleep(0.1)
            if timeout is not None and (_time.time() - start) > timeout:
                break
        return None

    def run_for(self, seconds):
        return self.loop(timeout=seconds)

    def close(self):
        self._stop = True
        try: self.sock.close()
        except Exception: pass
        return None

    def channels(self):
        return list(self._channels)


def namespace(interp):
    def _connect(args, kwargs):
        host = args[0] if len(args) > 0 else "irc.libera.chat"
        port = args[1] if len(args) > 1 else 6667
        nick = args[2] if len(args) > 2 else "GridBasic" + str(int(_time.time()) % 10000)
        use_tls = bool(args[3]) if len(args) > 3 else (int(port) == 6697)
        conn = IRCConnection(host, port, nick, use_tls, interp)
        conn._connect()
        return conn
    return {
        "connect": BoundBuiltin(_connect),
        "Connection": IRCConnection,
        "LIBERA": "irc.libera.chat:6697",
        "VERSION": "GridBasic IRC 1.0",
    }
