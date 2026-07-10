#!/usr/bin/env python3
"""Grid IRC host bridge — IRC for Grid OS guest at gateway:6667.

Grid OS connects to 10.0.2.2:6667 (QEMU user-net gateway). This bridge listens
on the host and either relays to a real IRC network or runs a local #grid hive
server for offline demos.

Environment:
  GRIDIRC_HOST      Listen address (default: 0.0.0.0)
  GRIDIRC_PORT      Listen port (default: 6667)
  GRIDIRC_MODE      local | relay | znc (default: local)
  GRIDIRC_UPSTREAM  Upstream host:port for relay/znc (default: irc.libera.chat:6667)
  GRIDIRC_ZNC_PASS  ZNC password when mode=znc (sent to upstream as PASS)

Examples:
  make irc-bridge
  GRIDIRC_MODE=relay GRIDIRC_UPSTREAM=irc.libera.chat:6667 make irc-bridge
  GRIDIRC_MODE=znc GRIDIRC_UPSTREAM=127.0.0.1:6697 GRIDIRC_ZNC_PASS=secret make irc-bridge
"""

from __future__ import annotations

import os
import socket
import threading
import time
from dataclasses import dataclass, field

CRLF = "\r\n"
SERVER_NAME = "gridirc.bridge"


def env_port(name: str, default: int) -> int:
    return int(os.environ.get(name, str(default)))


def parse_upstream(raw: str) -> tuple[str, int]:
    if ":" in raw:
        host, port_s = raw.rsplit(":", 1)
        return host, int(port_s)
    return raw, 6667


def send_line(sock: socket.socket, line: str) -> None:
    sock.sendall((line + CRLF).encode("utf-8", errors="replace"))


def relay_stream(src: socket.socket, dst: socket.socket) -> None:
    try:
        while True:
            data = src.recv(4096)
            if not data:
                break
            dst.sendall(data)
    except OSError:
        pass


def handle_relay_client(client: socket.socket, addr: tuple[str, int]) -> None:
    upstream_raw = os.environ.get("GRIDIRC_UPSTREAM", "irc.libera.chat:6667")
    host, port = parse_upstream(upstream_raw)
    mode = os.environ.get("GRIDIRC_MODE", "local").lower()
    znc_pass = os.environ.get("GRIDIRC_ZNC_PASS", "")
    print(f"[relay] {addr[0]}:{addr[1]} -> {host}:{port} mode={mode}", flush=True)
    upstream: socket.socket | None = None
    try:
        upstream = socket.create_connection((host, port), timeout=15)
        if mode == "znc" and znc_pass:
            send_line(upstream, f"PASS {znc_pass}")
        t1 = threading.Thread(target=relay_stream, args=(client, upstream), daemon=True)
        t2 = threading.Thread(target=relay_stream, args=(upstream, client), daemon=True)
        t1.start()
        t2.start()
        t1.join()
        t2.join()
    except OSError as exc:
        print(f"[relay] {addr[0]}:{addr[1]} error: {exc}", flush=True)
    finally:
        client.close()
        if upstream:
            upstream.close()


@dataclass
class LocalClient:
    sock: socket.socket
    addr: tuple[str, int]
    nick: str = ""
    user: str = ""
    channels: set[str] = field(default_factory=set)
    registered: bool = False


class LocalIRCServer:
    def __init__(self) -> None:
        self.clients: list[LocalClient] = []
        self.channel_members: dict[str, set[LocalClient]] = {"#grid": set()}
        self.lock = threading.Lock()
        self._hive_lines = [
            "The hive remembers.",
            "Consensus forming on #grid.",
            "Flynn's Grid is listening.",
            "End of line.",
        ]
        self._hive_index = 0

    def add_client(self, sock: socket.socket, addr: tuple[str, int]) -> None:
        client = LocalClient(sock=sock, addr=addr)
        with self.lock:
            self.clients.append(client)
        threading.Thread(target=self._client_loop, args=(client,), daemon=True).start()

    def _remove_client(self, client: LocalClient) -> None:
        with self.lock:
            if client in self.clients:
                self.clients.remove(client)
            for members in self.channel_members.values():
                members.discard(client)

    def _broadcast_channel(self, channel: str, line: str, skip: LocalClient | None = None) -> None:
        payload = (line + CRLF).encode("utf-8", errors="replace")
        with self.lock:
            for member in list(self.channel_members.get(channel, set())):
                if member is skip:
                    continue
                try:
                    member.sock.sendall(payload)
                except OSError:
                    self._remove_client(member)

    def _welcome(self, client: LocalClient) -> None:
        nick = client.nick or "guest"
        send_line(client.sock, f":{SERVER_NAME} 001 {nick} :Welcome to the Grid IRC hive")
        send_line(client.sock, f":{SERVER_NAME} 002 {nick} :Your host is {SERVER_NAME}")
        send_line(client.sock, f":{SERVER_NAME} 003 {nick} :Server created for Flynn's Grid")
        send_line(client.sock, f":{SERVER_NAME} 004 {nick} {SERVER_NAME} v1 gridirc")
        send_line(client.sock, f":{SERVER_NAME} 375 {nick} :- {SERVER_NAME} Message of the day -")
        send_line(client.sock, f":{SERVER_NAME} 372 {nick} :Join #grid and run irc-hive-mind.bas")
        send_line(client.sock, f":{SERVER_NAME} 376 {nick} :End of /MOTD command.")
        client.registered = True

    def _join(self, client: LocalClient, channel: str) -> None:
        if not channel.startswith("#"):
            channel = f"#{channel}"
        with self.lock:
            self.channel_members.setdefault(channel, set()).add(client)
        client.channels.add(channel)
        nick = client.nick or "guest"
        send_line(client.sock, f":{nick}!{client.user}@{SERVER_NAME} JOIN {channel}")
        send_line(client.sock, f":{SERVER_NAME} 353 {nick} = {channel} :{nick} hivebot griduser")
        send_line(client.sock, f":{SERVER_NAME} 366 {nick} {channel} :End of /NAMES list.")
        self._broadcast_channel(
            channel,
            f":{nick}!{client.user}@{SERVER_NAME} JOIN {channel}",
            skip=client,
        )
        if channel == "#grid":
            send_line(
                client.sock,
                f":hivebot!bot@{SERVER_NAME} PRIVMSG {channel} :hivemind local bridge online",
            )

    def _privmsg(self, client: LocalClient, target: str, text: str) -> None:
        nick = client.nick or "guest"
        line = f":{nick}!{client.user}@{SERVER_NAME} PRIVMSG {target} :{text}"
        if target.startswith("#"):
            self._broadcast_channel(target, line)
            if target == "#grid" and text.strip().lower().startswith("!hive"):
                reply = self._next_hive_line()
                send_line(
                    client.sock,
                    f":hivebot!bot@{SERVER_NAME} PRIVMSG {target} :{reply}",
                )
        else:
            send_line(client.sock, line)

    def _next_hive_line(self) -> str:
        line = self._hive_lines[self._hive_index % len(self._hive_lines)]
        self._hive_index += 1
        return line

    def _handle_line(self, client: LocalClient, line: str) -> None:
        line = line.strip("\r")
        if not line:
            return
        if line.startswith("PING"):
            token = line[5:].strip()
            send_line(client.sock, f"PONG :{token}")
            return
        if line.startswith("NICK "):
            client.nick = line[5:].strip()
            if client.user and not client.registered:
                self._welcome(client)
            return
        if line.startswith("USER "):
            parts = line.split(" ", 4)
            if len(parts) >= 2:
                client.user = parts[1]
            if client.nick and not client.registered:
                self._welcome(client)
            return
        if line.startswith("JOIN "):
            channel = line[5:].strip().split(" ")[0]
            self._join(client, channel)
            return
        if line.startswith("PART "):
            channel = line[5:].strip().split(" ")[0]
            with self.lock:
                self.channel_members.get(channel, set()).discard(client)
            client.channels.discard(channel)
            return
        if line.startswith("PRIVMSG "):
            rest = line[8:]
            if " :" in rest:
                target, text = rest.split(" :", 1)
                self._privmsg(client, target.strip(), text)
            return
        if line.startswith("QUIT"):
            self._remove_client(client)
            try:
                client.sock.close()
            except OSError:
                pass
            return

    def _client_loop(self, client: LocalClient) -> None:
        addr = client.addr
        print(f"[local] client {addr[0]}:{addr[1]}", flush=True)
        buf = ""
        try:
            while True:
                chunk = client.sock.recv(4096)
                if not chunk:
                    break
                buf += chunk.decode("utf-8", errors="replace")
                while CRLF in buf or "\n" in buf:
                    sep = CRLF if CRLF in buf else "\n"
                    line, buf = buf.split(sep, 1)
                    self._handle_line(client, line)
        except OSError:
            pass
        finally:
            self._remove_client(client)
            try:
                client.sock.close()
            except OSError:
                pass
            print(f"[local] disconnect {addr[0]}:{addr[1]}", flush=True)


def serve() -> None:
    host = os.environ.get("GRIDIRC_HOST", "0.0.0.0")
    port = env_port("GRIDIRC_PORT", 6667)
    mode = os.environ.get("GRIDIRC_MODE", "local").lower()
    local_server = LocalIRCServer() if mode == "local" else None

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((host, port))
    sock.listen(16)
    print(f"Grid IRC bridge listening on {host}:{port} mode={mode}", flush=True)
    if mode in {"relay", "znc"}:
        print(f"  upstream={os.environ.get('GRIDIRC_UPSTREAM', 'irc.libera.chat:6667')}", flush=True)
    else:
        print("  local #grid hive server — grid> irc connect gateway 6667 hivemind", flush=True)

    while True:
        client, addr = sock.accept()
        if mode == "local":
            local_server.add_client(client, addr)
        else:
            threading.Thread(target=handle_relay_client, args=(client, addr), daemon=True).start()


def main() -> None:
    if len(os.sys.argv) > 1 and os.sys.argv[1] in {"-h", "--help"}:
        print(__doc__)
        return
    serve()


if __name__ == "__main__":
    main()
