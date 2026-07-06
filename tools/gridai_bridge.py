#!/usr/bin/env python3
"""Grid AI host bridge — connects Grid OS GRIDAI protocol to an LLM API.

Grid OS sends framed requests over TCP (guest 10.0.2.2:8766) or COM1 serial.
Responses use the same GRIDAI/1.0 framing as kernel/ai.c.

Environment:
  GRIDAI_API_URL   OpenAI-compatible base URL (default: http://127.0.0.1:11434/v1)
  GRIDAI_MODEL     Model name (default: llama3.2)
  OPENAI_API_KEY   Bearer token when using OpenAI or compatible APIs
  GRIDAI_PORT      TCP listen port (default: 8766)
"""

from __future__ import annotations

import json
import os
import socket
import sys
import urllib.error
import urllib.request

END = "#GRIDAI/END"
HDR = "GRIDAI/1.0/"


def api_url() -> str:
    return os.environ.get("GRIDAI_API_URL", "http://127.0.0.1:11434/v1").rstrip("/")


def model_name() -> str:
    return os.environ.get("GRIDAI_MODEL", "llama3.2")


def system_for(action: str) -> str:
    base = (
        "You are Grid AI, assistant for GridBASIC on Grid OS 6.7. "
        "Answer concisely in plain text (no markdown fences). "
        "GridBASIC has PRINT LET IF FOR WHILE GOTO GOSUB DIM INPUT END and GRID.* bindings."
    )
    extra = {
        "ASK": "Answer the user's question about GridBASIC or general programming.",
        "EXPLAIN": "Explain the given GridBASIC line or snippet in one or two sentences.",
        "FIX": "Return corrected GridBASIC source only — no commentary.",
        "COMPLETE": "Return GridBASIC code that completes the fragment — source only.",
        "MODELS": "",
    }
    msg = extra.get(action, "")
    return f"{base} {msg}".strip()


def call_llm(action: str, user_text: str) -> str:
    if action == "MODELS":
        return f"model={model_name()} url={api_url()} bridge=gridai_bridge.py"

    messages = [
        {"role": "system", "content": system_for(action)},
        {"role": "user", "content": user_text or "(empty)"},
    ]
    payload = {
        "model": model_name(),
        "messages": messages,
        "temperature": 0.2,
        "max_tokens": 512,
    }
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        f"{api_url()}/chat/completions",
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    key = os.environ.get("OPENAI_API_KEY", "")
    if key:
        req.add_header("Authorization", f"Bearer {key}")

    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            body = json.loads(resp.read().decode("utf-8", errors="replace"))
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")[:200]
        raise RuntimeError(f"HTTP {exc.code}: {detail}") from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(f"LLM unreachable at {api_url()}: {exc}") from exc

    try:
        return body["choices"][0]["message"]["content"].strip()
    except (KeyError, IndexError, TypeError) as exc:
        raise RuntimeError(f"Unexpected LLM response: {body!r}") from exc


def parse_request(raw: str) -> tuple[str, str]:
    lines = raw.replace("\r\n", "\n").replace("\r", "\n").split("\n")
    if not lines:
        raise ValueError("empty request")
    header = lines[0].strip()
    if not header.startswith(HDR):
        raise ValueError(f"bad header: {header!r}")
    action = header[len(HDR) :].strip().upper()
    body_lines: list[str] = []
    for line in lines[1:]:
        if line.strip() == END:
            break
        body_lines.append(line)
    return action, "\n".join(body_lines).strip()


def format_ok(text: str) -> str:
    return f"GRIDAI/1.0/OK\n{text}\n{END}\n"


def format_err(text: str) -> str:
    return f"GRIDAI/1.0/ERR\n{text}\n{END}\n"


def handle_frame(raw: str) -> str:
    try:
        action, payload = parse_request(raw)
        if action not in {"ASK", "EXPLAIN", "FIX", "COMPLETE", "MODELS"}:
            return format_err(f"unknown action: {action}")
        reply = call_llm(action, payload)
        return format_ok(reply)
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
    print(f"Grid AI bridge listening on {host}:{port} -> {api_url()} model={model_name()}", flush=True)
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


def serve_serial() -> None:
    print(
        f"Grid AI bridge on stdio (serial) -> {api_url()} model={model_name()}",
        flush=True,
    )
    buf = ""
    while True:
        line = sys.stdin.readline()
        if not line:
            break
        buf += line
        if END in buf:
            resp = handle_frame(buf)
            sys.stdout.write(resp)
            sys.stdout.flush()
            buf = ""


def main() -> None:
    if len(sys.argv) > 1 and sys.argv[1] in {"-h", "--help"}:
        print(__doc__)
        return
    mode = os.environ.get("GRIDAI_MODE", "tcp")
    if len(sys.argv) > 1 and sys.argv[1] == "--serial":
        serve_serial()
        return
    host = os.environ.get("GRIDAI_HOST", "0.0.0.0")
    port = int(os.environ.get("GRIDAI_PORT", "8766"))
    if mode == "serial":
        serve_serial()
    else:
        serve_tcp(host, port)


if __name__ == "__main__":
    main()
