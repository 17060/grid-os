"""GridBasic IDE backend — a self-contained HTTP server.

Serves the single-page IDE frontend and a small JSON API for:

* listing / reading / writing files in the workspace,
* running GridBasic programs in a background thread,
* streaming program output and GRID GUI events to the browser (long-poll),
* feeding stdin lines back into a running program,
* stopping a run.

No third-party dependencies — only :mod:`http.server`, :mod:`json`,
:mod:`threading` and :mod:`queue`.

Run with ``python -m gridbasic --ide`` and open http://localhost:8765 in a
browser.
"""

from __future__ import annotations

import json
import os
import queue
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from ..interpreter import Interpreter
from ..errors import GridBasicError
from ..modules import grid as grid_module


_HERE = os.path.dirname(os.path.abspath(__file__))
STATIC_DIR = os.path.join(_HERE, "static")
DEFAULT_PORT = 8765


class Run:
    """A single program execution. Holds output/event queues and the thread."""

    def __init__(self, run_id, source, path, workdir):
        self.id = run_id
        self.source = source
        self.path = path
        self.workdir = workdir
        self.output_q: "queue.Queue[dict]" = queue.Queue()
        self.input_q: "queue.Queue[str]" = queue.Queue()
        self.grid_q: "queue.Queue[dict]" = queue.Queue()
        self.events: list[dict] = []  # buffered events for late pollers
        self.events_lock = threading.Lock()
        self.thread = None
        self.done = False
        self.error = None
        self.started_at = time.time()
        self.ended_at = None
        self.interp = None

    def push(self, event):
        with self.events_lock:
            self.events.append(event)
        self.output_q.put(event)

    def push_output(self, text):
        self.push({"type": "output", "text": text})

    def add_input(self, line):
        self.input_q.put(line)

    def finish(self, error=None):
        self.ended_at = time.time()
        self.done = True
        self.error = error
        self.push({"type": "end", "error": error, "duration": self.ended_at - self.started_at})
        # unregister the grid consumer
        try:
            grid_module.unregister_consumer(self.grid_q)
        except Exception:
            pass


class RunManager:
    def __init__(self, workdir):
        self.workdir = workdir
        self.runs: dict[str, Run] = {}
        self._lock = threading.Lock()
        self._counter = 0

    def new_run(self, source, path=None):
        with self._lock:
            self._counter += 1
            run_id = f"run-{self._counter}"
        run = Run(run_id, source, path, self.workdir)
        self.runs[run_id] = run
        self._start(run)
        return run

    def _start(self, run):
        grid_module.register_consumer(run.grid_q)

        def output_fn(text):
            run.push_output(text)

        def stdin_fn():
            try:
                line = run.input_q.get(timeout=120)
            except queue.Empty:
                return ""
            return line

        interp = Interpreter(output=output_fn, stdin=stdin_fn)
        run.interp = interp

        def worker():
            try:
                interp.run(run.source)
            except GridBasicError as e:
                run.push({"type": "error", "message": str(e)})
                run.finish(error=str(e))
                return
            except Exception as e:  # noqa: BLE001
                run.push({"type": "error", "message": f"Internal error: {e}"})
                run.finish(error=str(e))
                return
            run.push({"type": "status", "message": "Program ended."})
            run.finish()

        run.thread = threading.Thread(target=worker, daemon=True)
        run.thread.start()

    def stop(self, run_id):
        run = self.runs.get(run_id)
        if run and run.interp:
            run.interp.request_stop()
            return True
        return False

    def add_input(self, run_id, line):
        run = self.runs.get(run_id)
        if run:
            run.add_input(line)
            return True
        return False

    def get_events_since(self, run_id, since):
        run = self.runs.get(run_id)
        if not run:
            return None
        with run.events_lock:
            events = run.events[since:]
        # also drain any grid events and inject them
        drained = []
        while True:
            try:
                drained.append(run.grid_q.get_nowait())
            except queue.Empty:
                break
        # inject grid events into the buffer too, with sequential indices
        with run.events_lock:
            for ge in drained:
                evt = {"type": "grid", "cmd": ge}
                run.events.append(evt)
                events.append(evt)
        return events, len(run.events)

    def get_run(self, run_id):
        return self.runs.get(run_id)


# ---------------------------------------------------------------------------
# File helpers
# ---------------------------------------------------------------------------
def list_files(workdir, sub="", depth=0, max_depth=3):
    base = os.path.join(workdir, sub) if sub else workdir
    out = []
    try:
        for name in sorted(os.listdir(base)):
            if name.startswith(".") or name == "__pycache__":
                continue
            full = os.path.join(base, name)
            rel = os.path.join(sub, name) if sub else name
            if os.path.isdir(full):
                if depth < max_depth:
                    out.append({"name": name, "path": rel, "type": "dir", "children": list_files(workdir, rel, depth + 1, max_depth)})
            else:
                if name.endswith((".gb", ".bas", ".txt", ".md", ".json")):
                    out.append({"name": name, "path": rel, "type": "file"})
    except OSError:
        pass
    return out


# ---------------------------------------------------------------------------
# HTTP handler
# ---------------------------------------------------------------------------
class IDEHandler(BaseHTTPRequestHandler):
    manager: RunManager = None  # set on the class

    def log_message(self, *args):
        pass  # quiet

    def _send_json(self, code, obj):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        try:
            self.wfile.write(body)
        except Exception:
            pass

    def _send_text(self, code, text, ctype="text/plain; charset=utf-8"):
        body = text.encode("utf-8") if isinstance(text, str) else text
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        try:
            self.wfile.write(body)
        except Exception:
            pass

    def _body(self):
        length = int(self.headers.get("Content-Length", 0))
        if length:
            raw = self.rfile.read(length)
            return raw
        return b""

    def _body_json(self):
        raw = self._body()
        if not raw:
            return {}
        try:
            return json.loads(raw.decode("utf-8"))
        except Exception:
            return {}

    # ---- GET -----------------------------------------------------------
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        qs = urllib.parse.parse_qs(parsed.query)
        if path == "/" or path == "/index.html":
            self._serve_static("index.html", "text/html; charset=utf-8")
            return
        if path.startswith("/static/"):
            name = path[len("/static/"):]
            ctype = _content_type(name)
            self._serve_static(name, ctype)
            return
        if path == "/api/files":
            files = list_files(self.manager.workdir)
            self._send_json(200, {"files": files})
            return
        if path == "/api/file":
            rel = qs.get("path", [""])[0]
            full = self._safe_path(rel)
            if full is None:
                self._send_json(400, {"error": "invalid path"})
                return
            try:
                with open(full, "r", encoding="utf-8") as fh:
                    content = fh.read()
                self._send_json(200, {"path": rel, "content": content})
            except Exception as e:
                self._send_json(404, {"error": str(e)})
            return
        if path == "/api/examples":
            ex_dir = os.path.join(_HERE, "..", "..", "examples")
            ex_dir = os.path.abspath(ex_dir)
            exs = []
            try:
                for name in sorted(os.listdir(ex_dir)):
                    if name.endswith((".gb", ".bas")):
                        with open(os.path.join(ex_dir, name), "r", encoding="utf-8") as fh:
                            exs.append({"name": name, "content": fh.read()})
            except Exception:
                pass
            self._send_json(200, {"examples": exs})
            return
        if path == "/api/output":
            run_id = qs.get("id", [""])[0]
            since = int(qs.get("since", ["0"])[0])
            result = self.manager.get_events_since(run_id, since)
            if result is None:
                self._send_json(404, {"error": "unknown run"})
                return
            events, total = result
            self._send_json(200, {"events": events, "next": total})
            return
        if path == "/api/info":
            from .. import __version__
            self._send_json(200, {"version": __version__, "workdir": self.manager.workdir})
            return
        self._send_text(404, "not found")

    # ---- POST ----------------------------------------------------------
    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        if path == "/api/run":
            data = self._body_json()
            source = data.get("source", "")
            file_path = data.get("path")
            run = self.manager.new_run(source, file_path)
            self._send_json(200, {"id": run.id})
            return
        if path == "/api/save":
            data = self._body_json()
            rel = data.get("path", "")
            content = data.get("content", "")
            full = self._safe_path(rel)
            if full is None:
                self._send_json(400, {"error": "invalid path"})
                return
            try:
                os.makedirs(os.path.dirname(full), exist_ok=True)
                with open(full, "w", encoding="utf-8") as fh:
                    fh.write(content)
                self._send_json(200, {"ok": True, "path": rel})
            except Exception as e:
                self._send_json(500, {"error": str(e)})
            return
        if path == "/api/input":
            data = self._body_json()
            run_id = data.get("id", "")
            line = data.get("line", "")
            ok = self.manager.add_input(run_id, line)
            self._send_json(200 if ok else 404, {"ok": ok})
            return
        if path == "/api/stop":
            data = self._body_json()
            run_id = data.get("id", "")
            ok = self.manager.stop(run_id)
            self._send_json(200 if ok else 404, {"ok": ok})
            return
        self._send_text(404, "not found")

    # ---- helpers -------------------------------------------------------
    def _safe_path(self, rel):
        rel = rel.lstrip("/")
        full = os.path.abspath(os.path.join(self.manager.workdir, rel))
        if not full.startswith(self.manager.workdir):
            return None
        return full

    def _serve_static(self, name, ctype):
        path = os.path.join(STATIC_DIR, name)
        if not os.path.isfile(path):
            self._send_text(404, "not found")
            return
        try:
            with open(path, "rb") as fh:
                data = fh.read()
        except Exception as e:
            self._send_text(500, str(e))
            return
        self.send_response(200)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        try:
            self.wfile.write(data)
        except Exception:
            pass


def _content_type(name):
    if name.endswith(".html"): return "text/html; charset=utf-8"
    if name.endswith(".css"): return "text/css; charset=utf-8"
    if name.endswith(".js"): return "application/javascript; charset=utf-8"
    if name.endswith(".json"): return "application/json"
    if name.endswith(".png"): return "image/png"
    if name.endswith(".svg"): return "image/svg+xml"
    return "application/octet-stream"


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def serve(port=DEFAULT_PORT, workdir=None, open_browser=True):
    workdir = os.path.abspath(workdir or os.getcwd())
    manager = RunManager(workdir)
    IDEHandler.manager = manager
    httpd = ThreadingHTTPServer(("0.0.0.0", port), IDEHandler)
    url = f"http://localhost:{port}"
    banner = (
        "\n  GridBasic IDE\n"
        "  ==============\n"
        f"  Workspace : {workdir}\n"
        f"  URL       : {url}\n"
        "  Open the URL in a browser. Ctrl+C to stop.\n\n"
    )
    print(banner)
    if open_browser:
        try:
            import webbrowser, threading as _t
            _t.Thread(target=lambda: webbrowser.open(url), daemon=True).start()
        except Exception:
            pass
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")
        httpd.shutdown()
    return 0
