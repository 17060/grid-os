#!/usr/bin/env python3
"""Grid HTTPS host bridge — plain HTTP from Grid OS guest, TLS on the host.

Grid OS sends a raw HTTP/1.1 request over TCP to 10.0.2.2:8768 (default).
The bridge forwards it to https://<Host>/<path> using the Host header from
the guest request, then returns the upstream response bytes unchanged.

Environment:
  GRIDHTTPS_PORT   Listen port (default: 8768)
  GRIDHTTPS_HOST   Bind address (default: 0.0.0.0)
  GRIDHTTPS_TIMEOUT  Upstream timeout seconds (default: 60)
"""

from __future__ import annotations

import os
import socket
import ssl
import sys
import urllib.error
import urllib.request


def upstream_request(raw: bytes, timeout: float) -> bytes:
    text = raw.decode("utf-8", errors="replace")
    lines = text.split("\r\n")
    if not lines or " " not in lines[0]:
        raise ValueError("bad request line")
    method, target, _proto = lines[0].split(" ", 2)
    method = method.upper()
    if method not in {"GET", "POST", "PUT", "DELETE", "HEAD"}:
        raise ValueError(f"unsupported method: {method}")

    host = ""
    body_start = 0
    content_length = 0
    for i, line in enumerate(lines[1:], start=1):
        if line == "":
            body_start = i + 1
            break
        key, _, val = line.partition(":")
        if key.strip().lower() == "host":
            host = val.strip()
        if key.strip().lower() == "content-length":
            try:
                content_length = int(val.strip())
            except ValueError as exc:
                raise ValueError("bad Content-Length") from exc

    if not host:
        raise ValueError("missing Host header")

    body = ""
    if body_start:
        body = "\r\n".join(lines[body_start:])
    if content_length and len(body.encode("utf-8")) < content_length:
        body = text.split("\r\n\r\n", 1)[1] if "\r\n\r\n" in text else body

    if target.startswith("http://") or target.startswith("https://"):
        url = target
    else:
        url = f"https://{host}{target}"

    data = body.encode("utf-8") if body else None
    req = urllib.request.Request(url, data=data if method in {"POST", "PUT"} else None, method=method)
    req.add_header("Host", host.split(":")[0])
    req.add_header("User-Agent", "GridOS-HTTPS-Bridge/6.5")

    ctx = ssl.create_default_context()
    try:
        with urllib.request.urlopen(req, timeout=timeout, context=ctx) as resp:
            payload = resp.read()
            status = resp.status
            reason = resp.reason or "OK"
            headers = []
            for key, val in resp.headers.items():
                lk = key.lower()
                if lk in {"transfer-encoding", "connection", "content-encoding"}:
                    continue
                headers.append(f"{key}: {val}")
            header_block = "\r\n".join(headers)
            head = f"HTTP/1.1 {status} {reason}\r\n{header_block}\r\n\r\n"
            return head.encode("utf-8") + payload
    except urllib.error.HTTPError as exc:
        detail = exc.read()
        head = f"HTTP/1.1 {exc.code} {exc.reason}\r\nContent-Length: {len(detail)}\r\n\r\n"
        return head.encode("utf-8") + detail
    except urllib.error.URLError as exc:
        raise RuntimeError(f"upstream TLS failed for {url}: {exc}") from exc


def handle_client(conn: socket.socket, timeout: float) -> None:
    chunks: list[bytes] = []
    while True:
        part = conn.recv(4096)
        if not part:
            break
        chunks.append(part)
        joined = b"".join(chunks)
        if b"\r\n\r\n" in joined:
            header, rest = joined.split(b"\r\n\r\n", 1)
            clen = 0
            for line in header.decode("utf-8", errors="replace").split("\r\n"):
                if line.lower().startswith("content-length:"):
                    clen = int(line.split(":", 1)[1].strip())
            if len(rest) >= clen:
                break
        if sum(len(c) for c in chunks) > 65536:
            raise ValueError("request too large")
    raw = b"".join(chunks)
    resp = upstream_request(raw, timeout)
    conn.sendall(resp)


def serve(host: str, port: int, timeout: float) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((host, port))
    sock.listen(8)
    print(f"Grid HTTPS bridge listening on {host}:{port} (Grid OS 6.5)", flush=True)
    while True:
        conn, addr = sock.accept()
        with conn:
            try:
                handle_client(conn, timeout)
                print(f"[tcp {addr[0]}:{addr[1]}] proxied OK", flush=True)
            except Exception as exc:  # noqa: BLE001
                err = (
                    "HTTP/1.1 502 Bad Gateway\r\n"
                    "Content-Type: text/plain\r\n"
                    f"Content-Length: {len(str(exc))}\r\n\r\n{exc}"
                )
                conn.sendall(err.encode("utf-8"))
                print(f"[tcp {addr[0]}:{addr[1]}] error: {exc}", flush=True)


def main() -> None:
    if len(sys.argv) > 1 and sys.argv[1] in {"-h", "--help"}:
        print(__doc__)
        return
    host = os.environ.get("GRIDHTTPS_HOST", "0.0.0.0")
    port = int(os.environ.get("GRIDHTTPS_PORT", "8768"))
    timeout = float(os.environ.get("GRIDHTTPS_TIMEOUT", "60"))
    serve(host, port, timeout)


if __name__ == "__main__":
    main()
