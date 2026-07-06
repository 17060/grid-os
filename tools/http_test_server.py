#!/usr/bin/env python3
"""Tiny HTTP server for Grid OS keep-alive e2e tests."""

from __future__ import annotations

import os
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer


class Handler(BaseHTTPRequestHandler):
    hits = 0

    def do_GET(self) -> None:  # noqa: N802
        Handler.hits += 1
        body = f"KEEPALIVE_OK hits={Handler.hits}\n".encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "keep-alive")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt: str, *args) -> None:
        sys.stderr.write(f"[http-test] {self.address_string()} {fmt % args}\n")


def main() -> int:
    port = int(os.environ.get("GRID_HTTP_TEST_PORT", "18080"))
    host = os.environ.get("GRID_HTTP_TEST_HOST", "0.0.0.0")
    server = HTTPServer((host, port), Handler)
    print(f"HTTP test server on {host}:{port}", flush=True)
    server.serve_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
