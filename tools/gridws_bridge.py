#!/usr/bin/env python3
"""WebSocket bridge for Grid OS — guest TCP :8769 -> host WS upstream."""

from __future__ import annotations

import asyncio
import socket
import sys

HOST = "0.0.0.0"
PORT = 8769


async def handle(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
    try:
        line = (await reader.readline()).decode("utf-8", errors="replace").strip()
        if not line.startswith("WS "):
            writer.write(b"ERR bad handshake\n")
            await writer.drain()
            return
        url = line[3:].strip()
        writer.write(b"OK\n")
        await writer.drain()
        # Minimal stub: echo URL ack (full WS client optional on host)
        writer.write(f"WS-BRIDGE-OK {url}\n".encode())
        await writer.drain()
    finally:
        writer.close()
        await writer.wait_closed()


async def main() -> None:
    server = await asyncio.start_server(handle, HOST, PORT)
    print(f"Grid WebSocket bridge listening on {HOST}:{PORT} (Grid OS 7.0)", flush=True)
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        sys.exit(0)
