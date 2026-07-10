#!/usr/bin/env python3
"""Hive push relay — optional APNs companion for IRC Hive Mind iOS app.

Stores device tokens and accepts notify requests. Production deployments should
forward to Apple Push Notification service; this relay logs and acknowledges for
development and Grid OS lab workflows.

Environment:
  HIVE_PUSH_HOST   Listen address (default: 0.0.0.0)
  HIVE_PUSH_PORT   Listen port (default: 8770)

Endpoints:
  POST /register   {"token":"...","nick":"..."}
  POST /notify     {"token":"...","title":"...","body":"..."}
  POST /broadcast  {"title":"...","body":"..."}
  GET  /tokens     list registered tokens (dev only)

iOS Settings → Push relay URL: http://<your-mac-ip>:8770
"""

from __future__ import annotations

import json
import os
import socket
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any

TOKENS: dict[str, str] = {}
LOCK = threading.Lock()


class PushHandler(BaseHTTPRequestHandler):
    def log_message(self, fmt: str, *args: Any) -> None:
        print(f"[hive-push] {self.address_string()} {fmt % args}", flush=True)

    def _read_json(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(length) if length else b"{}"
        try:
            return json.loads(raw.decode("utf-8"))
        except json.JSONDecodeError:
            return {}

    def _json_response(self, code: int, payload: dict[str, Any]) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        if self.path == "/tokens":
            with LOCK:
                self._json_response(200, {"tokens": TOKENS})
            return
        if self.path == "/health":
            self._json_response(200, {"ok": True})
            return
        self._json_response(404, {"error": "not found"})

    def do_POST(self) -> None:
        data = self._read_json()
        if self.path == "/register":
            token = str(data.get("token", "")).strip()
            nick = str(data.get("nick", "hive")).strip()
            if not token:
                self._json_response(400, {"error": "token required"})
                return
            with LOCK:
                TOKENS[token] = nick
            self._json_response(200, {"registered": True, "count": len(TOKENS)})
            return

        if self.path in {"/notify", "/broadcast"}:
            title = str(data.get("title", "IRC Hive Mind"))
            body = str(data.get("body", ""))
            if self.path == "/notify":
                token = str(data.get("token", "")).strip()
                if not token:
                    self._json_response(400, {"error": "token required"})
                    return
                print(f"[push] -> {token[:12]}… title={title!r} body={body!r}", flush=True)
                self._json_response(200, {"sent": 1})
                return
            with LOCK:
                targets = list(TOKENS.keys())
            for token in targets:
                print(f"[push] broadcast {token[:12]}… {title!r}", flush=True)
            self._json_response(200, {"sent": len(targets)})
            return

        self._json_response(404, {"error": "not found"})


def main() -> None:
    host = os.environ.get("HIVE_PUSH_HOST", "0.0.0.0")
    port = int(os.environ.get("HIVE_PUSH_PORT", "8770"))
    server = ThreadingHTTPServer((host, port), PushHandler)
    print(f"Hive push relay on http://{host}:{port}", flush=True)
    print("  POST /register  POST /notify  POST /broadcast  GET /health", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
