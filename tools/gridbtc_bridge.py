#!/usr/bin/env python3
"""Grid BTC host bridge — connects Grid OS GRIDBTC protocol to Bitcoin Core RPC.

Grid OS sends framed requests over TCP (guest 10.0.2.2:8767).
Responses use the same GRIDBTC/1.0 framing as kernel/btc.c.

Environment:
  BITCOIN_RPC_URL      JSON-RPC URL (default: http://127.0.0.1:8332/)
  BITCOIN_RPC_USER     RPC username (required unless cookie auth works)
  BITCOIN_RPC_PASSWORD RPC password
  GRIDBTC_PORT         TCP listen port (default: 8767)
  GRIDBTC_HOST         Bind address (default: 0.0.0.0)
"""

from __future__ import annotations

import base64
import json
import os
import socket
import sys
import urllib.error
import urllib.request

END = "#GRIDBTC/END"
HDR = "GRIDBTC/1.0/"

HELP_TEXT = """Grid BTC bridge — common RPC methods (forwarded to bitcoind):
  getblockchaininfo  getnetworkinfo  getwalletinfo  getbalance
  getnewaddress      sendtoaddress   listtransactions  getblock
  getblockhash       getrawtransaction  estimatesmartfee  stop
  help               uptime          getpeerinfo       getmininginfo
Use btc call <method> [params-json] for any RPC method."""


def rpc_url() -> str:
    return os.environ.get("BITCOIN_RPC_URL", "http://127.0.0.1:8332/").rstrip("/") + "/"


def rpc_auth_header() -> str | None:
    user = os.environ.get("BITCOIN_RPC_USER", "")
    password = os.environ.get("BITCOIN_RPC_PASSWORD", "")
    if not user and not password:
        return None
    token = base64.b64encode(f"{user}:{password}".encode()).decode("ascii")
    return f"Basic {token}"


def bitcoin_rpc(method: str, params: list | dict | None, timeout: float = 60) -> object:
    payload = {"jsonrpc": "1.0", "id": "gridbtc", "method": method, "params": params or []}
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(rpc_url(), data=data, headers={"Content-Type": "application/json"}, method="POST")
    auth = rpc_auth_header()
    if auth:
        req.add_header("Authorization", auth)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            body = json.loads(resp.read().decode("utf-8", errors="replace"))
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")[:300]
        raise RuntimeError(f"Bitcoin RPC HTTP {exc.code}: {detail}") from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(
            f"Bitcoin Core unreachable at {rpc_url()} "
            f"(start bitcoind or set BITCOIN_RPC_URL): {exc}"
        ) from exc

    if body.get("error"):
        err = body["error"]
        if isinstance(err, dict):
            msg = err.get("message", str(err))
            code = err.get("code", "?")
            raise RuntimeError(f"RPC error {code}: {msg}")
        raise RuntimeError(f"RPC error: {err}")
    return body.get("result")


def parse_params(body: str) -> list | dict:
    text = body.strip()
    if not text:
        return []
    if text[0] in "[{":
        return json.loads(text)
    # key=value lines -> dict (simple convenience)
    out: dict[str, str] = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise ValueError(f"bad param line: {line!r}")
        key, val = line.split("=", 1)
        out[key.strip()] = val.strip()
    return out


def parse_request(raw: str) -> tuple[str, str]:
    lines = raw.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    if not lines:
        raise ValueError("empty request")
    header = lines[0].strip()
    if not header.startswith(HDR):
        raise ValueError(f"bad header: {header!r}")
    method = header[len(HDR) :].strip()
    body_lines: list[str] = []
    for line in lines[1:]:
        if line.strip() == END:
            break
        body_lines.append(line)
    return method, "\n".join(body_lines).strip()


def format_ok(text: str) -> str:
    return f"GRIDBTC/1.0/OK\n{text}\n{END}\n"


def format_err(text: str) -> str:
    return f"GRIDBTC/1.0/ERR\n{text}\n{END}\n"


def handle_special(method: str) -> str | None:
    upper = method.upper()
    if upper == "HELP":
        return HELP_TEXT
    if upper == "STATUS":
        try:
            info = bitcoin_rpc("getblockchaininfo", [], timeout=3)
            chain = info.get("chain", "?") if isinstance(info, dict) else "?"
            blocks = info.get("blocks", "?") if isinstance(info, dict) else "?"
            return f"bridge=gridbtc_bridge.py rpc={rpc_url()} connected=1 chain={chain} blocks={blocks}"
        except Exception as exc:  # noqa: BLE001
            return f"bridge=gridbtc_bridge.py rpc={rpc_url()} connected=0 error={exc}"
    return None


def handle_frame(raw: str) -> str:
    try:
        method, body = parse_request(raw)
        if not method:
            return format_err("missing RPC method")
        special = handle_special(method)
        if special is not None:
            return format_ok(special)
        params = parse_params(body)
        rpc_method = method.lower()
        result = bitcoin_rpc(rpc_method, params if isinstance(params, list) else [params])
        if isinstance(result, (dict, list)):
            text = json.dumps(result, separators=(",", ":"))
        elif result is None:
            text = "null"
        else:
            text = str(result)
        return format_ok(text)
    except Exception as exc:  # noqa: BLE001 — bridge returns errors to guest
        return format_err(str(exc))


def recv_frame_stream(stream) -> str:
    buf = ""
    while END not in buf:
        chunk = stream.read(4096)
        if not chunk:
            break
        if isinstance(chunk, bytes):
            buf += chunk.decode("utf-8", errors="replace")
        else:
            buf += chunk
    return buf


def serve_tcp(host: str, port: int) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((host, port))
    sock.listen(8)
    print(f"Grid BTC bridge listening on {host}:{port} -> {rpc_url()}", flush=True)
    while True:
        conn, addr = sock.accept()
        with conn:
            try:
                raw = recv_frame_stream(conn.makefile("rb"))
                if not raw.strip():
                    continue
                resp = handle_frame(raw)
                conn.sendall(resp.encode("utf-8"))
                print(f"[tcp {addr[0]}:{addr[1]}] {raw.splitlines()[0]} ok", flush=True)
            except Exception as exc:  # noqa: BLE001
                err = format_err(str(exc))
                conn.sendall(err.encode("utf-8"))
                print(f"[tcp {addr[0]}:{addr[1]}] error: {exc}", flush=True)


def main() -> None:
    if len(sys.argv) > 1 and sys.argv[1] in {"-h", "--help"}:
        print(__doc__)
        return
    host = os.environ.get("GRIDBTC_HOST", "0.0.0.0")
    port = int(os.environ.get("GRIDBTC_PORT", "8767"))
    serve_tcp(host, port)


if __name__ == "__main__":
    main()
