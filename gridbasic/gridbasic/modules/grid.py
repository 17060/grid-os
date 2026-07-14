"""GridBasic GRID module — the bridge between a running program and the IDE.

Programs emit high-level GUI/graphics commands (``GRID.plot``, ``GRID.color``,
``GRID.clear``, ``GRID.text``, ``GRID.alert``) which are pushed onto a
thread-safe queue. The IDE's backend drains this queue and forwards the
commands to the browser front-end over the websocket/SSE channel, so a
GridBasic program can draw on the IDE canvas in real time.

It also provides real HTTP networking (``GRID.http_get`` / ``GRID.http_post``)
backed by :mod:`urllib`.
"""

from __future__ import annotations

import json as _json
import queue as _queue
import threading
import urllib.request
import urllib.error
import urllib.parse
from concurrent.futures import ThreadPoolExecutor

from ..interpreter import BoundBuiltin, Future
from ..errors import GBRuntimeError


# A single process-wide event queue. The IDE backend polls this and forwards
# events to the connected browser.
_gui_events: "list[_queue.Queue]" = []
_gui_lock = threading.Lock()


def register_consumer(q: "_queue.Queue"):
    with _gui_lock:
        _gui_events.append(q)


def unregister_consumer(q: "_queue.Queue"):
    with _gui_lock:
        if q in _gui_events:
            _gui_events.remove(q)


def _push(event):
    with _gui_lock:
        for q in _gui_events:
            try:
                q.put_nowait(event)
            except Exception:
                pass


# ===========================================================================
# HTTP
# ===========================================================================
def _http_get(url, headers=None, timeout=15):
    req = urllib.request.Request(url, method="GET")
    if headers:
        for k, v in headers.items():
            req.add_header(k, str(v))
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            data = resp.read().decode("utf-8", "replace")
            return {"status": resp.status, "body": data,
                    "headers": dict(resp.headers)}
    except urllib.error.HTTPError as e:
        return {"status": e.code, "body": e.read().decode("utf-8", "replace"),
                "headers": {}}
    except Exception as e:
        return {"status": 0, "body": str(e), "headers": {}}


def _http_post(url, body, headers=None, timeout=15):
    if isinstance(body, (dict, list)):
        data = _json.dumps(body).encode("utf-8")
        h = dict(headers or {})
        h.setdefault("Content-Type", "application/json")
    elif isinstance(body, str):
        data = body.encode("utf-8")
        h = dict(headers or {})
    else:
        data = str(body).encode("utf-8")
        h = dict(headers or {})
    req = urllib.request.Request(url, data=data, method="POST")
    for k, v in h.items():
        req.add_header(k, str(v))
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return {"status": resp.status,
                    "body": resp.read().decode("utf-8", "replace"),
                    "headers": dict(resp.headers)}
    except urllib.error.HTTPError as e:
        return {"status": e.code, "body": e.read().decode("utf-8", "replace"),
                "headers": {}}
    except Exception as e:
        return {"status": 0, "body": str(e), "headers": {}}


# ===========================================================================
# Namespace
# ===========================================================================
def namespace(interp):
    def _gui(args, kwargs):
        _push({"type": "gui_show"})
        return True
    def _plot(args, kwargs):
        _push({"type": "plot", "x": args[0], "y": args[1]})
        return True
    def _line(args, kwargs):
        _push({"type": "line", "x1": args[0], "y1": args[1],
               "x2": args[2], "y2": args[3]})
        return True
    def _rect(args, kwargs):
        _push({"type": "rect", "x": args[0], "y": args[1],
               "w": args[2], "h": args[3]})
        return True
    def _circle(args, kwargs):
        _push({"type": "circle", "x": args[0], "y": args[1], "r": args[2]})
        return True
    def _color(args, kwargs):
        _push({"type": "color", "color": args[0]})
        return True
    def _clear(args, kwargs):
        _push({"type": "clear"})
        return True
    def _text(args, kwargs):
        _push({"type": "text", "x": args[0], "y": args[1], "text": args[2]})
        return True
    def _alert(args, kwargs):
        _push({"type": "alert", "message": args[0]})
        return True
    def _log(args, kwargs):
        _push({"type": "log", "message": args[0]})
        return True
    def _httpget(args, kwargs):
        return _http_get(args[0], kwargs.get("headers"))
    def _httppost(args, kwargs):
        return _http_post(args[0], args[1] if len(args) > 1 else "",
                          kwargs.get("headers"))
    def _httpget_async(args, kwargs):
        url = args[0]; headers = kwargs.get("headers")
        return Future(lambda: _http_get(url, headers))
    def _now(args, kwargs):
        import time
        return time.time()
    return {
        "gui": BoundBuiltin(_gui),
        "plot": BoundBuiltin(_plot),
        "line": BoundBuiltin(_line),
        "rect": BoundBuiltin(_rect),
        "circle": BoundBuiltin(_circle),
        "color": BoundBuiltin(_color),
        "clear": BoundBuiltin(_clear),
        "text": BoundBuiltin(_text),
        "alert": BoundBuiltin(_alert),
        "log": BoundBuiltin(_log),
        "http_get": BoundBuiltin(_httpget),
        "http_post": BoundBuiltin(_httppost),
        "http_get_async": BoundBuiltin(_httpget_async),
        "now": BoundBuiltin(_now),
        "VERSION": "GridBasic GRID 1.0",
    }
