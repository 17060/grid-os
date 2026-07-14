"""The GridBasic interpreter — a tree-walking evaluator.

Design highlights:

* Numbers, strings, bools, ``None``, lists, dicts, tuples, ranges reuse Python
  values; functions, classes, instances, channels, generators and futures are
  tiny runtime classes defined here.
* Environments form a parent chain. The global scope is shared across threads
  (locked); per-thread state (current env, call depth, step budget) lives in a
  ``threading.local`` so ``spawn``/``async``/generators run in isolation.
* Classic-BASIC ``GOTO``/``GOSUB``/``RETURN`` are implemented at the top level
  via control-flow signals and a label -> statement-index table.
* Concurrency: ``spawn``/``async`` run a callable in a thread and return a
  :class:`Future`; ``chan`` is a thread-safe bounded queue; generators run their
  body in a thread and lazily feed values through a size-1 queue.
"""

from __future__ import annotations

import math
import re as _re
import threading
import queue as _queue
import time as _time

from . import ast_nodes as ast
from .errors import (
    GridBasicError, GBRuntimeError, ReturnSignal, BreakSignal, ContinueSignal,
    ThrowSignal, YieldSignal, ParseError,
)
from . import stdlib as _stdlib


# ---- control-flow signals for top-level GOTO/GOSUB -----------------------
class GotoSignal(Exception):
    def __init__(self, target): self.target = target

class GosubCallSignal(Exception):
    def __init__(self, target): self.target = target

class GosubReturnSignal(Exception):
    pass

class StopSignal(Exception):
    pass


# ---- runtime value types -------------------------------------------------
class Function:
    __slots__ = ("name", "params", "body", "closure", "is_async", "is_generator",
                 "interp", "is_sub", "bound", "decorators")
    kind = "function"
    def __init__(self, name, params, body, closure, interp, is_async=False,
                 is_generator=False, is_sub=False, bound=None, decorators=None):
        self.name = name
        self.params = params
        self.body = body
        self.closure = closure
        self.interp = interp
        self.is_async = is_async
        self.is_generator = is_generator
        self.is_sub = is_sub
        self.bound = bound  # instance for methods
        self.decorators = decorators or []

    def bind(self, instance):
        return Function(self.name, self.params, self.body, self.closure,
                        self.interp, self.is_async, self.is_generator,
                        self.is_sub, instance, self.decorators)

    def __repr__(self):
        return f"<function {self.name or 'anonymous'}>"


class NativeFunction:
    __slots__ = ("name", "fn", "arity")
    kind = "native"
    def __init__(self, name, fn, arity=None):
        self.name = name; self.fn = fn; self.arity = arity

    def __repr__(self):
        return f"<native {self.name}>"


class ClassObj:
    __slots__ = ("name", "base", "fields", "methods", "ctor", "statics", "decorators")
    kind = "class"
    def __init__(self, name, base, fields, methods, ctor, statics, decorators=None):
        self.name = name
        self.base = base  # ClassObj or None
        self.fields = fields  # list of (name, default_expr)
        self.methods = methods  # dict name -> Function
        self.ctor = ctor  # (params, body) or None
        self.statics = statics  # dict name -> value
        self.decorators = decorators or []

    def find_method(self, name):
        m = self.methods.get(name)
        if m is not None:
            return m
        if self.base is not None:
            return self.base.find_method(name)
        return None

    def find_field_default(self, name):
        for fname, default in self.fields:
            if fname == name:
                return default
        if self.base is not None:
            return self.base.find_field_default(name)
        return None

    def all_field_names(self):
        names = []
        if self.base is not None:
            names = self.base.all_field_names()
        for fname, _ in self.fields:
            if fname not in names:
                names.append(fname)
        return names

    def __repr__(self):
        return f"<class {self.name}>"


class Instance:
    __slots__ = ("cls", "fields")
    kind = "instance"
    def __init__(self, cls):
        self.cls = cls
        self.fields = {}

    def __repr__(self):
        return f"<{self.cls.name} instance>"


class Channel:
    __slots__ = ("q", "closed", "lock")
    kind = "channel"
    def __init__(self, size=0):
        self.q = _queue.Queue(maxsize=max(1, size))
        self.closed = False
        self.lock = threading.Lock()

    def send(self, value, timeout=None):
        if self.closed:
            raise GBRuntimeError("send on closed channel")
        self.q.put(("v", value), timeout=timeout)
        return None

    def recv(self, timeout=None):
        item = self.q.get(timeout=timeout)
        if item[0] == "done":
            raise GBRuntimeError("channel closed")
        if item[0] == "err":
            raise item[1]
        return item[1]

    def close(self):
        with self.lock:
            if not self.closed:
                self.closed = True
                self.q.put(("done", None))

    def __repr__(self):
        return f"<channel {self.q.qsize()}>"


_SENTINEL = object()


class Generator:
    """A lazy generator. Its body runs in a thread; each ``yield`` enqueues a
    value through a bounded channel so consumption drives production."""
    __slots__ = ("thread", "ch", "started", "interp_ctx")
    kind = "generator"
    def __init__(self):
        self.ch = Channel(1)
        self.thread = None
        self.started = False

    def _start(self, run_body):
        def target():
            try:
                run_body()
            except ThrowSignal as t:
                self.ch.q.put(("err", GBRuntimeError(_safe_str(t.value))))
            except GBRuntimeError as e:
                self.ch.q.put(("err", e))
            except ReturnSignal:
                pass
            except Exception as e:  # noqa: BLE001
                self.ch.q.put(("err", GBRuntimeError(str(e))))
            finally:
                try:
                    self.ch.q.put(("done", None))
                except Exception:
                    pass
        self.thread = threading.Thread(target=target, daemon=True)
        self.thread.start()
        self.started = True

    def __iter__(self):
        return self

    def __next__(self):
        if not self.started:
            raise GBRuntimeError("generator not started")
        try:
            return self.ch.recv()
        except GBRuntimeError:
            raise StopIteration

    def next(self):
        try:
            return self.__next__()
        except StopIteration:
            raise GBRuntimeError("generator exhausted")

    def __repr__(self):
        return "<generator>"


class Future:
    __slots__ = ("thread", "result", "error", "done", "lock", "cond")
    kind = "future"
    def __init__(self, target):
        self.result = None
        self.error = None
        self.done = False
        self.lock = threading.Lock()
        self.cond = threading.Condition(self.lock)
        self.thread = threading.Thread(target=self._run, args=(target,), daemon=True)
        self.thread.start()

    def _run(self, target):
        try:
            self.result = target()
        except ThrowSignal as t:
            self.error = GBRuntimeError(_safe_str(t.value))
        except GBRuntimeError as e:
            self.error = e
        except Exception as e:  # noqa: BLE001
            self.error = GBRuntimeError(str(e))
        finally:
            with self.cond:
                self.done = True
                self.cond.notify_all()

    def await_result(self, timeout=None):
        with self.cond:
            while not self.done:
                self.cond.wait(timeout=timeout)
        if self.error is not None:
            raise self.error
        return self.result

    def __repr__(self):
        return f"<future {'done' if self.done else 'pending'}>"


# ---- environment ---------------------------------------------------------
class Env:
    __slots__ = ("vars", "parent", "lock", "is_global")
    def __init__(self, parent=None, is_global=False):
        self.vars = {}
        self.parent = parent
        self.lock = threading.RLock()
        self.is_global = is_global

    def get(self, name):
        env = self
        while env is not None:
            with env.lock:
                if name in env.vars:
                    return env.vars[name]
            env = env.parent
        return _SENTINEL

    def set_existing(self, name, value):
        """Assign to an existing binding in the nearest scope that has it.
        Returns True if found, else False."""
        env = self
        while env is not None:
            with env.lock:
                if name in env.vars:
                    env.vars[name] = value
                    return True
            env = env.parent
        return False

    def define(self, name, value):
        with self.lock:
            self.vars[name] = value

    def get_scope(self, name):
        env = self
        while env is not None:
            with env.lock:
                if name in env.vars:
                    return env
            env = env.parent
        return None


def _safe_str(v):
    try:
        return str(v)
    except Exception:
        return repr(v)


# ---- the interpreter -----------------------------------------------------
class Interpreter:
    MAX_DEPTH = 200

    def __init__(self, output=None, stdin=None, max_steps=5_000_000):
        self.output = output  # callable(str) or None (defaults to print)
        self.stdin = stdin    # callable() -> str or None
        self.max_steps = max_steps
        self.globals = Env(is_global=True)
        self.labels = {}
        self._tl = threading.local()
        self._print_lock = threading.Lock()
        self.modules = {}  # name -> module namespace dict
        self._option_base = 0
        self._data_pointer = 0
        self._data_values = []
        self._on_error_label = None
        self._stop_event = threading.Event()
        self._setup_stdlib()
        self._load_builtin_modules()

    def request_stop(self):
        self._stop_event.set()

    # ---- per-thread state ------------------------------------------------
    def _ensure_thread(self):
        tl = self._tl
        if not hasattr(tl, "env"):
            tl.env = self.globals
            tl.depth = 0
            tl.steps = 0
            tl.in_function = False
            tl.me = _SENTINEL
            tl.gen_chan = None

    @property
    def env(self):
        self._ensure_thread()
        return self._tl.env

    @env.setter
    def env(self, v):
        self._tl.env = v

    @property
    def depth(self):
        self._ensure_thread()
        return self._tl.depth

    @depth.setter
    def depth(self, v):
        self._tl.depth = v

    @property
    def steps(self):
        self._ensure_thread()
        return self._tl.steps

    @steps.setter
    def steps(self, v):
        self._tl.steps = v

    @property
    def in_function(self):
        self._ensure_thread()
        return self._tl.in_function

    @in_function.setter
    def in_function(self, v):
        self._tl.in_function = v

    def _init_thread(self):
        self._ensure_thread()

    # ---- setup -----------------------------------------------------------
    def _setup_stdlib(self):
        for name, fn in _stdlib.functions(self).items():
            self.globals.define(name, NativeFunction(name, fn))
        # chan(size) -> Channel  (used by the CHAN keyword in parse_primary)
        def _make_chan(args, kwargs):
            size = int(args[0]) if args else 0
            return Channel(size)
        self.globals.define("__chan__", NativeFunction("__chan__", _make_chan))

    def _load_builtin_modules(self):
        from .modules import irc as irc_mod
        from .modules import crypto as crypto_mod
        from .modules import ai as ai_mod
        from .modules import grid as grid_mod
        self.modules["irc"] = irc_mod.namespace(self)
        self.modules["crypto"] = crypto_mod.namespace(self)
        self.modules["ai"] = ai_mod.namespace(self)
        self.modules["grid"] = grid_mod.namespace(self)
        self.modules["math"] = _stdlib.math_namespace()
        self.modules["string"] = _stdlib.string_namespace()
        self.modules["time"] = _stdlib.time_namespace()
        self.modules["json"] = _stdlib.json_namespace()
        self.modules["io"] = _stdlib.io_namespace()
        # expose module names as identifiers too (so `IRC.connect` works)
        for mname in ("irc", "crypto", "ai", "grid", "math", "string", "time", "json", "io"):
            self.globals.define(mname.upper(), self.modules[mname])
            self.globals.define(mname, self.modules[mname])

    # ---- public entry ----------------------------------------------------
    def run(self, source: str):
        from .lexer import Lexer
        from .parser import Parser
        self._init_thread()
        tokens = Lexer(source).tokenize()
        program = Parser(tokens).parse()
        self.exec_program(program)

    def run_ast(self, program):
        self._init_thread()
        self.exec_program(program)

    # ---- output ----------------------------------------------------------
    def emit(self, text: str):
        with self._print_lock:
            if self.output is not None:
                self.output(text)
            else:
                import sys
                sys.stdout.write(text)
                sys.stdout.flush()

    def read_input(self, prompt=""):
        if prompt:
            self.emit(prompt)
        if self.stdin is not None:
            return self.stdin()
        import sys
        sys.stdout.flush()
        line = sys.stdin.readline()
        if line == "":
            return ""
        return line.rstrip("\n")

    # ---- program execution with GOTO/GOSUB -------------------------------
    def exec_program(self, program):
        self._init_thread()
        self.labels = program.labels
        ip = 0
        gosub_stack = []
        body = program.body
        n = len(body)
        while ip < n:
            stmt = body[ip]
            try:
                self.exec_stmt(stmt)
            except GotoSignal as g:
                if g.target not in self.labels:
                    raise GBRuntimeError(f"Undefined label: {g.target}", stmt.line if hasattr(stmt,'line') else 0)
                ip = self.labels[g.target]
                continue
            except GosubCallSignal as g:
                if g.target not in self.labels:
                    raise GBRuntimeError(f"Undefined label: {g.target}")
                gosub_stack.append(ip + 1)
                ip = self.labels[g.target]
                continue
            except GosubReturnSignal:
                if not gosub_stack:
                    raise GBRuntimeError("RETURN without GOSUB")
                ip = gosub_stack.pop()
                continue
            except StopSignal:
                return
            ip += 1

    # ---- statement dispatch ----------------------------------------------
    def exec_stmt(self, node):
        self._tick()
        k = node.kind
        method = self._stmt_dispatch.get(k)
        if method is None:
            raise GBRuntimeError(f"Unknown statement: {k}", getattr(node, 'line', 0))
        method(self, node)

    def _tick(self):
        if self._stop_event.is_set():
            raise StopSignal()
        self.steps = self.steps + 1
        if self.steps > self.max_steps:
            raise GBRuntimeError(f"Step limit exceeded ({self.max_steps}). Possible infinite loop.")
        if self.depth > self.MAX_DEPTH:
            raise GBRuntimeError("Maximum call depth exceeded")

    # ---- expression dispatch ---------------------------------------------
    def eval(self, node):
        k = node.kind
        method = self._expr_dispatch.get(k)
        if method is None:
            raise GBRuntimeError(f"Unknown expression: {k}", getattr(node, 'line', 0))
        return method(self, node)

    # ---- statements ------------------------------------------------------
    def s_exprstmt(self, n):
        self.eval(n.expr)

    def s_label(self, n):
        pass  # labels are handled by the program executor

    def s_pass(self, n):
        pass

    def s_stop(self, n):
        raise StopSignal()

    def s_print(self, n):
        if n.using is not None:
            fmt = self.eval(n.using)
            vals = [self.eval(p[0]) for p in n.parts]
            self.emit(_format_using(fmt, vals))
            if n.newline:
                self.emit("\n")
            return
        if not n.parts:
            if n.newline:
                self.emit("\n")
            return
        out = []
        for i, (expr, sep) in enumerate(n.parts):
            v = self.eval(expr)
            out.append(display(v))
            if sep == "," and i < len(n.parts) - 1:
                out.append("\t")
            elif sep == ";" and i < len(n.parts) - 1:
                out.append(" ")
        self.emit("".join(out))
        if n.newline:
            self.emit("\n")

    def s_input(self, n):
        prompt = ""
        if n.prompt is not None:
            prompt = display(self.eval(n.prompt))
        line = self.read_input(prompt)
        # parse the input into the target variable(s)
        self.assign_target(n.var, _parse_input_value(line))

    def s_let(self, n):
        if n.value is None:
            self.define_var(n.name, None)
            return
        v = self.eval(n.value)
        v = self._maybe_const(n, v)
        self.define_var(n.name, v)

    def _maybe_const(self, n, v):
        if n.is_const:
            return _ConstValue(v)
        return v

    def s_destructure(self, n):
        v = self.eval(n.value)
        if n.flavor == "list":
            if not isinstance(v, (list, tuple)):
                raise GBRuntimeError("Cannot destructure non-list")
            idx = 0
            for entry in n.pattern:
                if entry[0] == "rest":
                    self.define_var(entry[1], list(v[idx:]))
                    break
                self.define_var(entry[1], v[idx] if idx < len(v) else None)
                idx += 1
        else:
            if not isinstance(v, dict):
                raise GBRuntimeError("Cannot destructure non-dict")
            for entry in n.pattern:
                _, key, nm = entry
                self.define_var(nm, v.get(key))

    def s_dim(self, n):
        dims = [int(self.eval(d)) for d in n.dims]
        if len(dims) == 1:
            size = dims[0] + 1 - self._option_base
            self.define_var(n.name, [None] * max(0, size))
        elif len(dims) == 2:
            r = dims[0] + 1 - self._option_base
            c = dims[1] + 1 - self._option_base
            self.define_var(n.name, [[None] * max(0, c) for _ in range(max(0, r))])
        else:
            self.define_var(n.name, [])

    def s_block(self, n):
        for s in n.body:
            self.exec_stmt(s)

    def s_if(self, n):
        for cond, body in n.branches:
            if _truthy(self.eval(cond)):
                self._exec_block(body)
                return
        if n.else_body is not None:
            self._exec_block(n.else_body)

    def _exec_block(self, body):
        for s in body:
            self.exec_stmt(s)

    def s_for(self, n):
        start = self.eval(n.start)
        end = self.eval(n.end)
        step = self.eval(n.step) if n.step is not None else 1
        if not isinstance(start, (int, float)) or not isinstance(end, (int, float)):
            raise GBRuntimeError("FOR requires numeric bounds")
        self.define_var(n.var, start)
        while True:
            cur = self.get_var(n.var)
            if step >= 0 and cur > end:
                break
            if step < 0 and cur < end:
                break
            try:
                self._exec_block(n.body)
            except BreakSignal:
                return
            except ContinueSignal:
                pass
            self.assign_name(n.var, cur + step)

    def s_forin(self, n):
        it = self.eval(n.iter)
        for item in _iterate(it):
            self.define_var(n.var, item)
            try:
                self._exec_block(n.body)
            except BreakSignal:
                return
            except ContinueSignal:
                continue

    def s_while(self, n):
        while _truthy(self.eval(n.cond)):
            try:
                self._exec_block(n.body)
            except BreakSignal:
                return
            except ContinueSignal:
                continue

    def s_repeat(self, n):
        while True:
            try:
                self._exec_block(n.body)
            except BreakSignal:
                return
            except ContinueSignal:
                pass
            if _truthy(self.eval(n.cond)):
                break

    def s_select(self, n):
        val = self.eval(n.expr)
        for vals, guard, body in n.cases:
            matched = False
            for ve in vals:
                cv = self.eval(ve)
                if _equals(val, cv):
                    matched = True
                    break
            if matched and guard is not None and not _truthy(self.eval(guard)):
                matched = False
            if matched:
                self._exec_block(body)
                return
        # range-style: if vals is a RangeLit, check membership
        for vals, guard, body in n.cases:
            for ve in vals:
                if isinstance(ve, ast.RangeLit):
                    lo = self.eval(ve.start); hi = self.eval(ve.end)
                    if _in_range(val, lo, hi, ve.inclusive):
                        if guard is None or _truthy(self.eval(guard)):
                            self._exec_block(body)
                            return
        if n.else_body is not None:
            self._exec_block(n.else_body)

    def s_match(self, n):
        val = self.eval(n.scrutinee)
        for pat, guard, body in n.arms:
            bindings = _match_pattern(pat, val)
            if bindings is not None:
                if guard is not None:
                    scope = Env(self.env)
                    for k, v in bindings.items():
                        scope.define(k, v)
                    saved = self.env
                    self.env = scope
                    try:
                        ok = _truthy(self.eval(guard))
                    except Exception:
                        ok = False
                    finally:
                        self.env = saved
                    if not ok:
                        continue
                for k, v in bindings.items():
                    self.define_var(k, v)
                self._exec_block(body)
                return
        raise GBRuntimeError(f"Match failed for value {display(val)}", n.line)

    def s_func(self, n):
        fn = Function(n.name, n.params, n.body, self.env, self,
                      is_async=n.is_async, is_generator=n.is_generator,
                      decorators=n.decorators)
        fn = self._apply_decorators(fn, n.decorators)
        self.define_var(n.name, fn)

    def s_sub(self, n):
        fn = Function(n.name, n.params, n.body, self.env, self,
                      is_async=False, is_generator=False, is_sub=True,
                      decorators=n.decorators)
        fn = self._apply_decorators(fn, n.decorators)
        self.define_var(n.name, fn)

    def _apply_decorators(self, fn, decorators):
        for (name, args, kwargs) in reversed(decorators):
            dec = self.get_var(name)
            if dec is None or not callable_value(dec):
                raise GBRuntimeError(f"Unknown decorator: {name}")
            arg_vals = [self.eval(a) for a in args]
            kw_vals = {k: self.eval(v) for k, v in kwargs.items()}
            fn = self.call_value(dec, [fn] + arg_vals, kw_vals)
        return fn

    def s_return(self, n):
        val = self.eval(n.value) if n.value is not None else None
        if self.in_function:
            raise ReturnSignal(val)
        # top-level RETURN = return from GOSUB
        raise GosubReturnSignal()

    def s_break(self, n):
        raise BreakSignal()

    def s_continue(self, n):
        raise ContinueSignal()

    def s_yield(self, n):
        val = self.eval(n.value) if n.value is not None else None
        chan = getattr(self._tl, "gen_chan", None)
        if chan is not None:
            chan.send(val)   # blocks until the consumer takes it; preserves stack
            return
        raise GBRuntimeError("yield outside a generator", n.line)

    def s_throw(self, n):
        val = self.eval(n.value)
        raise ThrowSignal(val)

    def s_try(self, n):
        try:
            self._exec_block(n.body)
        except ThrowSignal as ts:
            exc_val = ts.value
            handled = False
            for (name, type_filter, cbody) in n.catches:
                if type_filter is None or _instance_of_name(exc_val, type_filter):
                    scope = Env(self.env)
                    if name:
                        scope.define(name, exc_val)
                    saved = self.env
                    self.env = scope
                    try:
                        self._exec_block(cbody)
                    finally:
                        self.env = saved
                    handled = True
                    break
            if not handled:
                if n.finally_body is not None:
                    self._exec_block(n.finally_body)
                raise
        finally:
            if n.finally_body is not None:
                self._exec_block(n.finally_body)

    def s_class(self, n):
        base = None
        if n.base is not None:
            base = self.eval(n.base)
            if not isinstance(base, ClassObj):
                raise GBRuntimeError("Base class not found")
        fields = []
        methods = {}
        ctor = None
        statics = {}
        defenv = self.env
        for member in n.members:
            kind = member[0]
            if kind == "field":
                _, fname, type_ann, default, is_static = member
                if is_static:
                    statics[fname] = self.eval(default) if default is not None else None
                else:
                    fields.append((fname, default))
            elif kind == "method":
                _, fn, is_static = member
                fobj = Function(fn.name, fn.params, fn.body, defenv, self,
                                is_async=fn.is_async, is_generator=fn.is_generator,
                                is_sub=(fn.kind == "sub"), decorators=fn.decorators)
                if is_static:
                    statics[fn.name] = fobj
                else:
                    methods[fn.name] = fobj
            elif kind == "ctor":
                _, params, body, is_static = member
                ctor = (params, body)
            elif kind == "nested":
                _, nested = member
                self.define_var(nested.name, self._build_class(nested))
        cls = ClassObj(n.name, base, fields, methods, ctor, statics, n.decorators)
        cls = self._apply_decorators_cls(cls, n.decorators)
        self.define_var(n.name, cls)

    def _build_class(self, n):
        base = None
        if n.base is not None:
            base = self.eval(n.base)
        fields = []
        methods = {}
        ctor = None
        statics = {}
        for member in n.members:
            kind = member[0]
            if kind == "field":
                _, fname, _t, default, is_static = member
                if is_static:
                    statics[fname] = self.eval(default) if default is not None else None
                else:
                    fields.append((fname, default))
            elif kind == "method":
                _, fn, is_static = member
                methods[fn.name] = fn if not is_static else None
                if is_static:
                    statics[fn.name] = fn
            elif kind == "ctor":
                _, params, body, _ = member
                ctor = (params, body)
        return ClassObj(n.name, base, fields, methods, ctor, statics, n.decorators)

    def _apply_decorators_cls(self, cls, decorators):
        for (name, args, kwargs) in reversed(decorators):
            dec = self.get_var(name)
            if dec is None or not callable_value(dec):
                raise GBRuntimeError(f"Unknown decorator: {name}")
            arg_vals = [self.eval(a) for a in args]
            kw_vals = {k: self.eval(v) for k, v in kwargs.items()}
            cls = self.call_value(dec, [cls] + arg_vals, kw_vals)
        return cls

    def s_enum(self, n):
        d = {}
        auto = 0
        for (vname, vexpr) in n.variants:
            if vexpr is not None:
                val = self.eval(vexpr)
            else:
                val = auto
            d[vname] = val
            if isinstance(val, int):
                auto = val + 1
            else:
                auto += 1
        # expose as a namespace dict
        self.define_var(n.name, _EnumNamespace(n.name, d))

    def s_import(self, n):
        mod = self.modules.get(n.module)
        if mod is None:
            # try to load a GridBasic source module from disk
            mod = self._load_source_module(n.module)
            if mod is None:
                raise GBRuntimeError(f"Unknown module: {n.module}")
        if n.alias:
            self.define_var(n.alias, mod)
            return
        if n.names == ["*"]:
            for k, v in mod.items():
                if not k.startswith("_"):
                    self.define_var(k, v)
            return
        if n.names:
            for nm in n.names:
                if nm in mod:
                    self.define_var(nm, mod[nm])
                else:
                    raise GBRuntimeError(f"Module {n.module} has no member {nm}")
        else:
            self.define_var(n.module, mod)

    def _load_source_module(self, name):
        import os
        for base in (os.getcwd(), os.path.join(os.path.dirname(__file__), "..", "lib")):
            path = os.path.join(base, name + ".gb")
            if os.path.isfile(path):
                with open(path, "r", encoding="utf-8") as fh:
                    src = fh.read()
                from .lexer import Lexer
                from .parser import Parser
                prog = Parser(Lexer(src).tokenize()).parse()
                mod_env = Env(self.globals)
                saved = self.env
                self.env = mod_env
                try:
                    for s in prog.body:
                        try:
                            self.exec_stmt(s)
                        except StopSignal:
                            break
                finally:
                    self.env = saved
                ns = dict(mod_env.vars)
                self.modules[name] = ns
                return ns
        return None

    def s_export(self, n):
        pass  # exports are implicit in module loading

    def s_goto(self, n):
        raise GotoSignal(n.target)

    def s_gosub(self, n):
        raise GosubCallSignal(n.target)

    def s_data(self, n):
        for v in n.values:
            if isinstance(v, ast.StrLit):
                self._data_values.append(v.value)
            else:
                self._data_values.append(self.eval(v))

    def s_read(self, n):
        for target in n.vars:
            if self._data_pointer >= len(self._data_values):
                raise GBRuntimeError("READ past end of DATA")
            val = self._data_values[self._data_pointer]
            self._data_pointer += 1
            self.assign_target(target, val)

    def s_restore(self, n):
        self._data_pointer = 0

    def s_randomize(self, n):
        import random
        if n.seed is not None:
            random.seed(self.eval(n.seed))
        else:
            random.seed()

    def s_optionbase(self, n):
        self._option_base = int(self.eval(n.base))

    def s_defer(self, n):
        # register a deferred call to run when current function returns
        # we implement via a simple stack on the thread-local
        if not hasattr(self._tl, "defers"):
            self._tl.defers = []
        self._tl.defers.append(n.call)

    def _run_defers(self):
        if not hasattr(self._tl, "defers"):
            return
        while self._tl.defers:
            call = self._tl.defers.pop()
            try:
                self.eval(call)
            except Exception as e:  # noqa: BLE001
                self.emit(f"defer error: {e}\n")

    def s_spawn(self, n):
        # Run the call in a background thread (returns a Future, ignored here).
        self._spawn_call(n.call)

    def s_send(self, n):
        ch = self.eval(n.ch)
        val = self.eval(n.value)
        if not isinstance(ch, Channel):
            raise GBRuntimeError("send requires a channel")
        ch.send(val)

    def s_with(self, n):
        val = self.eval(n.expr)
        scope = Env(self.env)
        if n.as_name:
            scope.define(n.as_name, val)
        # expose members of val into scope (for `with` blocks using bare names)
        if isinstance(val, dict):
            for k, v in val.items():
                scope.define(k, v)
        elif isinstance(val, Instance):
            for k, v in val.fields.items():
                scope.define(k, v)
        saved = self.env
        self.env = scope
        try:
            self._exec_block(n.body)
        finally:
            self.env = saved

    def s_assign(self, n):
        v = self.eval(n.value)
        self.assign_target(n.target, v)

    def s_assignop(self, n):
        cur = self.eval(n.target)
        rhs = self.eval(n.value)
        v = _binop(n.op, cur, rhs, getattr(n, 'line', 0))
        self.assign_target(n.target, v)

    # ---- assignment helpers ----------------------------------------------
    def assign_target(self, target, value):
        k = target.kind
        if k == "ident":
            self.assign_name(target.name, value)
        elif k == "index":
            obj = self.eval(target.obj)
            idx = self.eval(target.index)
            _set_index(obj, idx, value)
        elif k == "slice":
            obj = self.eval(target.obj)
            start = self.eval(target.start) if target.start else None
            stop = self.eval(target.stop) if target.stop else None
            step = self.eval(target.step) if target.step else None
            if isinstance(obj, list):
                obj[slice(start, stop, step)] = value if isinstance(value, list) else [value]
            else:
                raise GBRuntimeError("Cannot slice-assign non-list")
        elif k == "member":
            obj = self.eval(target.obj)
            _set_member(obj, target.name, value)
        elif k == "tuple":
            if not isinstance(value, (list, tuple)):
                raise GBRuntimeError("Cannot destructure non-sequence")
            for i, sub in enumerate(target.items):
                self.assign_target(sub, value[i] if i < len(value) else None)
        else:
            raise GBRuntimeError(f"Invalid assignment target: {k}")

    def assign_name(self, name, value):
        if isinstance(value, _ConstValue):
            value = value.value
        # if there's an existing const binding, error
        scope = self.env.get_scope(name)
        if scope is not None:
            with scope.lock:
                cur = scope.vars.get(name)
                if isinstance(cur, _ConstValue):
                    raise GBRuntimeError(f"Cannot assign to const {name}")
                scope.vars[name] = value
            return
        # global fallback (classic BASIC implicit globals)
        self.globals.define(name, value)

    def define_var(self, name, value):
        # define in current scope
        self.env.define(name, value)

    def get_var(self, name):
        v = self.env.get(name)
        if v is _SENTINEL:
            # special identifiers
            if name == "me":
                return _SENTINEL  # handled by method call binding
            raise GBRuntimeError(f"Undefined variable: {name}")
        if isinstance(v, _ConstValue):
            return v.value
        return v

    # ---- expressions -----------------------------------------------------
    def e_int(self, n): return n.value
    def e_float(self, n): return n.value
    def e_str(self, n): return n.value
    def e_bool(self, n): return n.value
    def e_none(self, n): return None
    def e_regex(self, n):
        return _re.compile(n.value)

    def e_fstring(self, n):
        out = []
        for seg in n.segments:
            if seg[0] == "lit":
                out.append(seg[1])
            else:
                _, expr, fmt = seg
                v = self.eval(expr)
                s = display(v)
                if fmt:
                    s = _apply_fmt(s, v, fmt)
                out.append(s)
        return "".join(out)

    def e_ident(self, n):
        v = self.env.get(n.name)
        if v is _SENTINEL:
            # special: `me` resolved at call site; here it should have been bound
            if n.name == "me":
                me = getattr(self._tl, "me", _SENTINEL)
                if me is not _SENTINEL:
                    return me
                raise GBRuntimeError("'me' used outside a method")
            raise GBRuntimeError(f"Undefined variable: {n.name}", n.line)
        if isinstance(v, _ConstValue):
            return v.value
        return v

    def e_list(self, n):
        out = []
        for item in n.items:
            if item.kind == "spread":
                v = self.eval(item.expr)
                if isinstance(v, (list, tuple, range)):
                    out.extend(list(v))
                else:
                    out.append(v)
            else:
                out.append(self.eval(item))
        return out

    def e_dict(self, n):
        d = {}
        for k, v in n.pairs:
            key = self.eval(k)
            if v.kind == "spread":
                val = self.eval(v.expr)
                if isinstance(val, dict):
                    d.update(val)
                continue
            d[key] = self.eval(v)
        return d

    def e_tuple(self, n):
        return tuple(self.eval(item) for item in n.items)

    def e_range(self, n):
        lo = self.eval(n.start); hi = self.eval(n.end)
        if not isinstance(lo, int) or not isinstance(hi, int):
            # float range — materialize
            step = 1
            out = []
            x = lo
            if lo <= hi:
                while x <= hi if n.inclusive else x < hi:
                    out.append(x); x += step
            else:
                while x >= hi if n.inclusive else x > hi:
                    out.append(x); x -= step
            return out
        if n.inclusive:
            return range(lo, hi + 1)
        return range(lo, hi)

    def e_comprehension(self, n):
        if n.flavor == "list":
            result = []
            self._run_comp(n.clauses, 0, {}, lambda b: result.append(self._eval_in_scope(n.expr, b)))
            return result
        else:
            result = {}
            def add(b):
                k = self._eval_in_scope(n.key, b)
                v = self._eval_in_scope(n.value, b)
                result[k] = v
            self._run_comp(n.clauses, 0, {}, add)
            return result

    def _run_comp(self, clauses, idx, bindings, emit):
        if idx >= len(clauses):
            emit(bindings); return
        clause = clauses[idx]
        if clause[0] == "for":
            _, name, itexpr = clause
            it = self._eval_in_scope(itexpr, bindings)
            for item in _iterate(it):
                bindings2 = dict(bindings); bindings2[name] = item
                self._run_comp(clauses, idx + 1, bindings2, emit)
        else:
            _, cond = clause
            if _truthy(self._eval_in_scope(cond, bindings)):
                self._run_comp(clauses, idx + 1, bindings, emit)

    def _eval_in_scope(self, node, bindings):
        scope = Env(self.env)
        for k, v in bindings.items():
            scope.define(k, v)
        saved = self.env
        self.env = scope
        try:
            return self.eval(node)
        finally:
            self.env = saved

    def e_binop(self, n):
        l = self.eval(n.left); r = self.eval(n.right)
        return _binop(n.op, l, r, n.line)

    def e_unop(self, n):
        v = self.eval(n.operand)
        op = n.op
        if op == "-":
            return -v
        if op == "+":
            return +v
        if op == "not":
            return not _truthy(v)
        raise GBRuntimeError(f"Unknown unary op: {op}")

    def e_logical(self, n):
        l = self.eval(n.left)
        if n.op == "and":
            if not _truthy(l): return l if isinstance(l, bool) else False
            r = self.eval(n.right)
            return r if _truthy(r) else (r if not isinstance(r, bool) else False)
        if n.op == "or":
            if _truthy(l): return l
            return self.eval(n.right)
        if n.op == "xor":
            r = self.eval(n.right)
            return _truthy(l) != _truthy(r)

    def e_assign(self, n):
        # assignment as expression (walrus): returns the assigned value
        v = self.eval(n.value)
        self.assign_target(n.target, v)
        return v

    def e_assignop(self, n):
        cur = self.eval(n.target)
        rhs = self.eval(n.value)
        v = _binop(n.op, cur, rhs, n.line)
        self.assign_target(n.target, v)
        return v

    def e_call(self, n):
        # NEW Type(args) is encoded as Call(Ident "__new__", [TypeExpr, *args])
        if n.callee.kind == "ident" and n.callee.name == "__new__":
            cls = self.eval(n.args[0])
            args = []
            for a in n.args[1:]:
                if a.kind == "spread":
                    v = self.eval(a.expr)
                    if isinstance(v, (list, tuple, range)):
                        args.extend(list(v))
                    else:
                        args.append(v)
                else:
                    args.append(self.eval(a))
            kwargs = {k: self.eval(v) for k, v in n.kwargs.items()}
            return self.call_value(cls, args, kwargs, node=n)
        callee = self.eval(n.callee)
        args = []
        for a in n.args:
            if a.kind == "spread":
                v = self.eval(a.expr)
                if isinstance(v, (list, tuple, range)):
                    args.extend(list(v))
                else:
                    args.append(v)
            else:
                args.append(self.eval(a))
        kwargs = {k: self.eval(v) for k, v in n.kwargs.items()}
        return self.call_value(callee, args, kwargs, node=n)

    def e_index(self, n):
        obj = self.eval(n.obj)
        idx = self.eval(n.index)
        return _get_index(obj, idx)

    def e_slice(self, n):
        obj = self.eval(n.obj)
        start = self.eval(n.start) if n.start else None
        stop = self.eval(n.stop) if n.stop else None
        step = self.eval(n.step) if n.step else None
        try:
            return obj[slice(start, stop, step)]
        except Exception as e:  # noqa: BLE001
            raise GBRuntimeError(f"Slice error: {e}")

    def e_member(self, n):
        obj = self.eval(n.obj)
        return _get_member(obj, n.name, self)

    def e_optmember(self, n):
        obj = self.eval(n.obj)
        if obj is None:
            return None
        return _get_member(obj, n.name, self)

    def e_lambda(self, n):
        if n.expr_form:
            body = [ast.ReturnStmt(n.body)]
        else:
            body = n.body if isinstance(n.body, list) else [n.body]
        return Function("", n.params, body, self.env, self, is_async=False)

    def e_ternary(self, n):
        if _truthy(self.eval(n.cond)):
            return self.eval(n.then)
        return self.eval(n.els)

    def e_nullish(self, n):
        l = self.eval(n.left)
        if l is None:
            return self.eval(n.right)
        return l

    def e_await(self, n):
        v = self.eval(n.expr)
        if isinstance(v, Future):
            return v.await_result()
        # awaiting a channel receives from it
        if isinstance(v, Channel):
            return v.recv()
        return v

    def e_chanrecv(self, n):
        ch = self.eval(n.chan)
        if not isinstance(ch, Channel):
            raise GBRuntimeError("receive requires a channel")
        return ch.recv()

    def e_matchexpr(self, n):
        val = self.eval(n.scrutinee)
        for pat, guard, body in n.arms:
            bindings = _match_pattern(pat, val)
            if bindings is not None:
                if guard is not None:
                    scope = Env(self.env)
                    for k, v in bindings.items():
                        scope.define(k, v)
                    saved = self.env; self.env = scope
                    try:
                        ok = _truthy(self.eval(guard))
                    finally:
                        self.env = saved
                    if not ok:
                        continue
                scope = Env(self.env)
                for k, v in bindings.items():
                    scope.define(k, v)
                saved = self.env; self.env = scope
                try:
                    return self.eval(body)
                finally:
                    self.env = saved
        raise GBRuntimeError("Match expression failed", n.line)

    def e_spawn(self, n):
        # spawn as expression -> run callable in thread, return Future
        call = n.call if hasattr(n, 'call') else n.expr
        return self._spawn_call(call)

    def _spawn_call(self, call_node):
        closure_env = self.env
        interp = self
        if isinstance(call_node, ast.Call):
            callee = self.eval(call_node.callee)
            args = []
            for a in call_node.args:
                if a.kind == "spread":
                    v = self.eval(a.expr)
                    if isinstance(v, (list, tuple, range)):
                        args.extend(list(v))
                    else:
                        args.append(v)
                else:
                    args.append(self.eval(a))
            kwargs = {k: self.eval(v) for k, v in call_node.kwargs.items()}

            def work():
                saved = interp.env
                interp.env = closure_env
                try:
                    return interp.call_value(callee, args, kwargs)
                finally:
                    interp.env = saved
            return Future(work)
        # expression that yields a callable (e.g. a lambda) — evaluate & call it
        def work2():
            saved = interp.env
            interp.env = closure_env
            try:
                fn = interp.eval(call_node)
                if callable_value(fn):
                    return interp.call_value(fn, [], {})
                return fn
            finally:
                interp.env = saved
        return Future(work2)

    def e_send(self, n):
        ch = self.eval(n.chan)
        val = self.eval(n.value)
        if not isinstance(ch, Channel):
            raise GBRuntimeError("send requires a channel")
        ch.send(val)
        return None

    def e_spread(self, n):
        return self.eval(n.expr)

    # ---- calling ---------------------------------------------------------
    def call_value(self, callee, args, kwargs, node=None):
        if isinstance(callee, NativeFunction):
            return callee.fn(args, kwargs)
        if isinstance(callee, Function):
            return self._call_user_function(callee, args, kwargs)
        if isinstance(callee, ClassObj):
            return self._instantiate(callee, args, kwargs)
        if isinstance(callee, BoundBuiltin):
            return callee(args, kwargs)
        if callable(callee):
            # a Python callable (e.g. method of a runtime object)
            try:
                return callee(*args, **{k: v for k, v in kwargs.items()})
            except TypeError:
                return callee(args, kwargs)
        raise GBRuntimeError(f"Value is not callable: {display(callee)}",
                             getattr(node, 'line', 0) if node else 0)

    def _call_user_function(self, fn, args, kwargs):
        self.depth += 1
        saved_env = self.env
        saved_me = getattr(self._tl, "me", _SENTINEL)
        saved_in_function = self.in_function
        try:
            local = Env(fn.closure)
            self._bind_params(fn, args, kwargs, local)
            if fn.bound is not None:
                local.define("me", fn.bound)
                self._tl.me = fn.bound
            else:
                self._tl.me = _SENTINEL
            self.env = local
            self.in_function = True
            if fn.is_generator:
                return self._make_generator(fn)
            if fn.is_async:
                return self._make_future(fn)
            try:
                self._exec_block(fn.body)
            except ReturnSignal as r:
                return r.value
            finally:
                self._run_defers()
            return None
        finally:
            self.env = saved_env
            self._tl.me = saved_me
            self.in_function = saved_in_function
            self.depth -= 1

    def _bind_params(self, fn, args, kwargs, local):
        params = fn.params
        nparam = len(params)
        # collect star/starstar
        star_idx = None
        for i, p in enumerate(params):
            if p[2] == "star":
                star_idx = i
        ai = 0
        for i, p in enumerate(params):
            name, default, kind, _t = p
            if kind == "star":
                # gather extra positional
                local.define(name, list(args[ai:]))
                star_idx = i
                # remaining params after star take from kwargs
                continue
            if ai < len(args):
                local.define(name, args[ai]); ai += 1
            elif name in kwargs:
                local.define(name, kwargs.pop(name))
            elif default is not None:
                local.define(name, self.eval(default))
            else:
                local.define(name, None)

    def _make_generator(self, fn):
        gen = Generator()
        interp = self
        closure_env = self.env
        def run_body():
            saved = interp.env
            interp.env = closure_env
            interp._ensure_thread()
            interp._tl.gen_chan = gen.ch
            interp.depth = 0
            interp.steps = 0
            interp.in_function = True
            try:
                try:
                    for s in fn.body:
                        interp.exec_stmt(s)
                except ReturnSignal:
                    pass
            finally:
                interp.env = saved
        gen._start(run_body)
        return gen

    def _make_future(self, fn):
        interp = self
        closure_env = self.env
        def work():
            saved = interp.env
            interp.env = closure_env
            interp.depth = 0
            interp.steps = 0
            interp.in_function = True
            try:
                try:
                    interp._exec_block(fn.body)
                except ReturnSignal as r:
                    return r.value
                return None
            finally:
                interp.env = saved
        return Future(work)

    def _instantiate(self, cls, args, kwargs):
        inst = Instance(cls)
        # init fields from class hierarchy
        c = cls
        order = []
        while c is not None:
            order.insert(0, c)
            c = c.base
        for c in order:
            for (fname, default) in c.fields:
                if default is not None:
                    val = self.eval(default)
                else:
                    val = None
                inst.fields[fname] = val
        # run constructor chain
        ctor = cls.ctor
        if ctor is None and cls.base is not None:
            # call base constructor with same args
            base_ctor = cls.base.ctor
            if base_ctor is not None:
                ctor = base_ctor
        if ctor is not None:
            params, body = ctor
            local = Env(self.globals)
            self._bind_params_ctor(params, args, kwargs, local, inst)
            saved = self.env
            saved_me = getattr(self._tl, "me", _SENTINEL)
            self.env = local
            local.define("me", inst)
            self._tl.me = inst
            try:
                self._exec_block(body)
            except ReturnSignal:
                pass
            finally:
                self.env = saved
                self._tl.me = saved_me
        return inst

    def _bind_params_ctor(self, params, args, kwargs, local, inst):
        ai = 0
        for p in params:
            name, default, kind, _t = p
            if ai < len(args):
                local.define(name, args[ai]); ai += 1
            elif name in kwargs:
                local.define(name, kwargs.pop(name))
            elif default is not None:
                local.define(name, self.eval(default))
            else:
                local.define(name, None)


# ---- helpers -------------------------------------------------------------
class _ConstValue:
    __slots__ = ("value",)
    def __init__(self, value): self.value = value


class _EnumNamespace:
    kind = "enum"
    def __init__(self, name, d):
        self.name = name
        self.d = d
    def __getitem__(self, k): return self.d[k]
    def __repr__(self): return f"<enum {self.name}>"


class BoundBuiltin:
    """A Python callable bound for use as a GridBasic callable."""
    def __init__(self, fn):
        self.fn = fn
    def __call__(self, args, kwargs):
        return self.fn(args, kwargs)


def callable_value(v):
    return isinstance(v, (Function, NativeFunction, ClassObj, BoundBuiltin)) or callable(v)


def _truthy(v):
    if v is None: return False
    if isinstance(v, bool): return v
    if isinstance(v, (int, float)): return v != 0
    if isinstance(v, str): return len(v) > 0
    if isinstance(v, (list, tuple, dict, range)): return len(v) > 0
    return True


def _equals(a, b):
    return a == b


def _in_range(val, lo, hi, inclusive):
    try:
        if inclusive:
            return lo <= val <= hi
        return lo <= val < hi
    except TypeError:
        return False


def _iterate(v):
    if isinstance(v, (list, tuple, range)):
        return iter(v)
    if isinstance(v, dict):
        return iter(v.keys())
    if isinstance(v, str):
        return iter(v)
    if isinstance(v, Generator):
        return iter(v)
    if v is None:
        return iter([])
    raise GBRuntimeError(f"Value is not iterable: {display(v)}")


def _binop(op, l, r, line=0):
    try:
        if op == "+":
            if isinstance(l, str) and isinstance(r, str):
                return l + r
            if isinstance(l, list) and isinstance(r, list):
                return l + r
            if isinstance(l, dict) and isinstance(r, dict):
                return {**l, **r}
            return l + r
        if op == "-": return l - r
        if op == "*":
            if isinstance(l, str) and isinstance(r, int): return l * r
            if isinstance(l, list) and isinstance(r, int): return l * r
            return l * r
        if op == "/":
            if r == 0:
                raise GBRuntimeError("Division by zero", line)
            return l / r
        if op == "%":
            if r == 0:
                raise GBRuntimeError("Modulo by zero", line)
            return l % r
        if op == "^":
            return l ** r
        if op == "mod":
            if r == 0:
                raise GBRuntimeError("Modulo by zero", line)
            return l % r
        if op == "div":
            if r == 0:
                raise GBRuntimeError("Integer division by zero", line)
            return l // r
        if op == "shl": return l << r
        if op == "shr": return l >> r
        if op == "==": return _equals(l, r)
        if op == "!=": return not _equals(l, r)
        if op == "<": return l < r
        if op == ">": return l > r
        if op == "<=": return l <= r
        if op == ">=": return l >= r
        if op == "in":
            if isinstance(r, dict): return l in r
            if isinstance(r, (list, tuple, range, str)): return l in r
            if isinstance(r, Generator):
                return any(_equals(l, x) for x in r)
            raise GBRuntimeError(f"Cannot use 'in' on {display(r)}")
        if op == "is":
            if isinstance(r, ast.NoneLit) or r is None:
                return l is None
            return l is r
    except TypeError as e:
        raise GBRuntimeError(f"Type error in '{op}': {e}", line)
    except GBRuntimeError:
        raise
    except Exception as e:  # noqa: BLE001
        raise GBRuntimeError(f"Error in '{op}': {e}", line)
    raise GBRuntimeError(f"Unknown operator: {op}", line)


def _get_index(obj, idx):
    if isinstance(obj, (list, tuple, str, range)):
        try:
            return obj[idx]
        except IndexError:
            raise GBRuntimeError(f"Index out of range: {idx}")
    if isinstance(obj, dict):
        if idx in obj:
            return obj[idx]
        return None
    if isinstance(obj, Instance):
        if idx in obj.fields:
            return obj.fields[idx]
        raise GBRuntimeError(f"No field {idx} on {obj.cls.name}")
    raise GBRuntimeError(f"Cannot index {display(obj)}")


def _set_index(obj, idx, value):
    if isinstance(obj, list):
        try:
            obj[idx] = value
        except IndexError:
            # auto-extend
            while len(obj) <= idx:
                obj.append(None)
            obj[idx] = value
    elif isinstance(obj, dict):
        obj[idx] = value
    elif isinstance(obj, Instance):
        obj.fields[idx] = value
    else:
        raise GBRuntimeError(f"Cannot index-assign {display(obj)}")


def _get_member(obj, name, interp):
    if isinstance(obj, Instance):
        if name in obj.fields:
            return obj.fields[name]
        m = obj.cls.find_method(name)
        if m is not None:
            return m.bind(obj) if isinstance(m, Function) else m
        raise GBRuntimeError(f"No member {name} on {obj.cls.name}")
    if isinstance(obj, ClassObj):
        if name in obj.statics:
            return obj.statics[name]
        m = obj.find_method(name)
        if m is not None:
            return m
        raise GBRuntimeError(f"No static member {name} on {obj.cls.name}")
    if isinstance(obj, dict):
        if name in obj:
            return obj[name]
        # method-like accessors for dict
        return _dict_method(obj, name, interp)
    if isinstance(obj, list):
        return _list_method(obj, name, interp)
    if isinstance(obj, str):
        return _str_method(obj, name, interp)
    if isinstance(obj, _EnumNamespace):
        if name in obj.d:
            return obj.d[name]
        raise GBRuntimeError(f"No variant {name} in enum {obj.name}")
    # module namespace dict — member access
    if isinstance(obj, dict) and "__module__" in obj:
        if name in obj:
            return obj[name]
    # Python object attribute fallback (for runtime values like regex, etc.)
    if hasattr(obj, name):
        attr = getattr(obj, name)
        if callable(attr):
            return BoundBuiltin(lambda args, kwargs, _a=attr: _a(*args, **kwargs))
        return attr
    raise GBRuntimeError(f"No member {name} on {display(obj)}")


def _set_member(obj, name, value):
    if isinstance(obj, Instance):
        obj.fields[name] = value
    elif isinstance(obj, dict):
        obj[name] = value
    else:
        raise GBRuntimeError(f"Cannot set member {name} on {display(obj)}")


def _dict_method(d, name, interp=None):
    import json as _json
    if name == "keys": return BoundBuiltin(lambda a, k: list(d.keys()))
    if name == "values": return BoundBuiltin(lambda a, k: list(d.values()))
    if name == "items": return BoundBuiltin(lambda a, k: [list(t) for t in d.items()])
    if name == "get": return BoundBuiltin(lambda a, k: d.get(a[0], a[1] if len(a) > 1 else None))
    if name == "has": return BoundBuiltin(lambda a, k: a[0] in d)
    if name == "size" or name == "len": return BoundBuiltin(lambda a, k: len(d))
    if name == "to_json": return BoundBuiltin(lambda a, k: _json.dumps(_pyify(d)))
    if name == "merge": return BoundBuiltin(lambda a, k: {**d, **(a[0] if a else {})})
    if name == "map" and interp is not None:
        def _dmap(a, k):
            fn = a[0]
            return {key: interp.call_value(fn, [val], {}) for key, val in d.items()}
        return BoundBuiltin(_dmap)
    if name == "filter" and interp is not None:
        def _dfilter(a, k):
            fn = a[0]
            return {key: val for key, val in d.items() if _truthy(interp.call_value(fn, [val], {}))}
        return BoundBuiltin(_dfilter)
    if name == "each" and interp is not None:
        def _deach(a, k):
            fn = a[0]
            for key, val in d.items():
                interp.call_value(fn, [key, val], {})
            return None
        return BoundBuiltin(_deach)
    raise GBRuntimeError(f"No member {name} on dict")


def _list_method(lst, name, interp=None):
    if name == "push" or name == "append":
        return BoundBuiltin(lambda a, k: (lst.append(a[0]), None)[1])
    if name == "pop":
        return BoundBuiltin(lambda a, k: lst.pop() if not a else lst.pop(a[0]))
    if name == "shift":
        return BoundBuiltin(lambda a, k: lst.pop(0))
    if name == "unshift":
        return BoundBuiltin(lambda a, k: (lst.insert(0, a[0]), None)[1])
    if name == "insert":
        return BoundBuiltin(lambda a, k: (lst.insert(int(a[0]), a[1]), None)[1])
    if name == "remove":
        return BoundBuiltin(lambda a, k: (lst.remove(a[0]), None)[1] if a[0] in lst else None)
    if name == "reverse":
        return BoundBuiltin(lambda a, k: (lst.reverse(), lst)[1])
    if name == "sort":
        def _sort(a, k):
            if a and isinstance(a[0], (Function, NativeFunction, BoundBuiltin)):
                key = lambda x: _truthy_cmp(interp.call_value(a[0], [x], {}))
            else:
                key = lambda x: x
            lst.sort(key=key)
            return lst
        return BoundBuiltin(_sort)
    if name == "map" and interp is not None:
        def _map(a, k):
            fn = a[0]
            return [interp.call_value(fn, [x], {}) for x in lst]
        return BoundBuiltin(_map)
    if name == "filter" and interp is not None:
        def _filter(a, k):
            fn = a[0]
            return [x for x in lst if _truthy(interp.call_value(fn, [x], {}))]
        return BoundBuiltin(_filter)
    if name == "reduce" and interp is not None:
        def _reduce(a, k):
            fn = a[0]
            it = iter(lst)
            if len(a) > 1:
                acc = a[1]
            else:
                acc = next(it)
            for x in it:
                acc = interp.call_value(fn, [acc, x], {})
            return acc
        return BoundBuiltin(_reduce)
    if name == "each" and interp is not None:
        def _each(a, k):
            fn = a[0]
            for x in lst:
                interp.call_value(fn, [x], {})
            return None
        return BoundBuiltin(_each)
    if name == "find" and interp is not None:
        def _find(a, k):
            fn = a[0]
            for x in lst:
                if _truthy(interp.call_value(fn, [x], {})):
                    return x
            return None
        return BoundBuiltin(_find)
    if name == "every" and interp is not None:
        def _every(a, k):
            fn = a[0]
            return all(_truthy(interp.call_value(fn, [x], {})) for x in lst)
        return BoundBuiltin(_every)
    if name == "some" and interp is not None:
        def _some(a, k):
            fn = a[0]
            return any(_truthy(interp.call_value(fn, [x], {})) for x in lst)
        return BoundBuiltin(_some)
    if name == "join":
        return BoundBuiltin(lambda a, k: (a[0] if a else ",").join(display(x) for x in lst))
    if name == "contains":
        return BoundBuiltin(lambda a, k: a[0] in lst)
    if name == "index":
        return BoundBuiltin(lambda a, k: lst.index(a[0]) if a[0] in lst else -1)
    if name == "slice":
        return BoundBuiltin(lambda a, k: lst[a[0]:a[1]] if len(a) > 1 else lst[a[0]:])
    if name == "concat":
        return BoundBuiltin(lambda a, k: lst + (a[0] if a else []))
    if name == "size" or name == "len":
        return BoundBuiltin(lambda a, k: len(lst))
    if name == "first":
        return BoundBuiltin(lambda a, k: lst[0] if lst else None)
    if name == "last":
        return BoundBuiltin(lambda a, k: lst[-1] if lst else None)
    if name == "flat":
        def _flat(a, k):
            out = []
            for x in lst:
                if isinstance(x, list): out.extend(x)
                else: out.append(x)
            return out
        return BoundBuiltin(_flat)
    if name == "sum":
        return BoundBuiltin(lambda a, k: sum(x for x in lst if isinstance(x, (int, float)) and not isinstance(x, bool)) if any(isinstance(x,(int,float)) for x in lst) else 0)
    if name == "min":
        return BoundBuiltin(lambda a, k: min((x for x in lst if x is not None), default=None))
    if name == "max":
        return BoundBuiltin(lambda a, k: max((x for x in lst if x is not None), default=None))
    raise GBRuntimeError(f"No member {name} on list")


def _truthy_cmp(v):
    """A sort key that handles mixed types by falling back to string form."""
    if v is None: return (0, 0)
    if isinstance(v, bool): return (1, int(v))
    if isinstance(v, (int, float)): return (2, v)
    if isinstance(v, str): return (3, v)
    return (4, str(v))


def _str_method(s, name, interp=None):
    def m0(fn):  # wrap a no-arg string method as a callable
        return BoundBuiltin(lambda a, k: fn())
    if name == "upper": return m0(s.upper)
    if name == "lower": return m0(s.lower)
    if name == "trim": return m0(s.strip)
    if name == "ltrim": return m0(s.lstrip)
    if name == "rtrim": return m0(s.rstrip)
    if name == "length" or name == "len" or name == "size":
        return m0(lambda: len(s))
    if name == "split":
        return BoundBuiltin(lambda a, k: s.split(a[0]) if a else s.split())
    if name == "replace":
        return BoundBuiltin(lambda a, k: s.replace(a[0], a[1]) if len(a) > 1 else s.replace(a[0], ""))
    if name == "contains":
        return BoundBuiltin(lambda a, k: a[0] in s)
    if name == "starts_with":
        return BoundBuiltin(lambda a, k: s.startswith(a[0]))
    if name == "ends_with":
        return BoundBuiltin(lambda a, k: s.endswith(a[0]))
    if name == "find":
        return BoundBuiltin(lambda a, k: s.find(a[0]))
    if name == "substr":
        return BoundBuiltin(lambda a, k: s[a[0]:a[1]] if len(a) > 1 else s[a[0]:])
    if name == "repeat":
        return BoundBuiltin(lambda a, k: s * int(a[0]))
    if name == "reverse":
        return m0(lambda: s[::-1])
    if name == "char_at":
        return BoundBuiltin(lambda a, k: s[int(a[0])])
    if name == "to_int":
        return BoundBuiltin(lambda a, k: int(s))
    if name == "to_float":
        return BoundBuiltin(lambda a, k: float(s))
    if name == "chars":
        return m0(lambda: list(s))
    if name == "match":
        def _m(a, k):
            import re
            m = re.search(a[0], s)
            if not m: return None
            return list(m.groups()) if m.groups() else [m.group(0)]
        return BoundBuiltin(_m)
    raise GBRuntimeError(f"No member {name} on string")


def _call_any(fn, args):
    if isinstance(fn, (Function, NativeFunction, BoundBuiltin)):
        # we need an interpreter — use the current one via threadlocal trick
        # BoundBuiltin already wraps a python callable
        if isinstance(fn, BoundBuiltin):
            return fn(args, {})
        # for user functions we need the interpreter; look it up
        raise GBRuntimeError("map/filter require an interpreter context")
    if callable(fn):
        return fn(*args)
    raise GBRuntimeError("Not callable")


def _pyify(v):
    if isinstance(v, tuple): return [_pyify(x) for x in v]
    if isinstance(v, list): return [_pyify(x) for x in v]
    if isinstance(v, dict): return {k: _pyify(x) for k, x in v.items()}
    if isinstance(v, Instance): return {k: _pyify(x) for k, x in v.fields.items()}
    return v


def _match_pattern(pat, val):
    k = pat.kind
    if k == "wildpat":
        return {}
    if k == "litpat":
        return {} if _equals(val, pat.value) else None
    if k == "bindpat":
        return {pat.name: val}
    if k == "orpat":
        for sub in pat.subs:
            b = _match_pattern(sub, val)
            if b is not None:
                return b
        return None
    if k == "tuplepat":
        if not isinstance(val, (list, tuple)) or len(val) != len(pat.subs):
            return None
        out = {}
        for sub, v in zip(pat.subs, val):
            b = _match_pattern(sub, v)
            if b is None: return None
            out.update(b)
        return out
    if k == "ctorpat":
        # treat as: value is instance of class pat.name with matching fields
        if isinstance(val, Instance) and val.cls.name == pat.name:
            out = {}
            fnames = val.cls.all_field_names()
            if len(pat.args) != len(fnames):
                return None
            for sub, fname in zip(pat.args, fnames):
                b = _match_pattern(sub, val.fields.get(fname))
                if b is None: return None
                out.update(b)
            return out
        # also: a dict with matching keys
        if isinstance(val, dict) and pat.name in ("dict", "Dict", "Map"):
            out = {}
            for i, sub in enumerate(pat.args):
                if i >= len(val): return None
                b = _match_pattern(sub, list(val.values())[i])
                if b is None: return None
                out.update(b)
            return out
        return None
    if k == "ispat":
        if _instance_of_name(val, pat.type_name):
            return {pat.name: val}
        return None
    if k == "rangepat":
        lo = _eval_const(pat.lo); hi = _eval_const(pat.hi)
        if _in_range(val, lo, hi, pat.inclusive):
            return {}
        return None
    return None


def _eval_const(node):
    if isinstance(node, ast.IntLit): return node.value
    if isinstance(node, ast.FloatLit): return node.value
    if isinstance(node, ast.StrLit): return node.value
    return None


def _instance_of_name(val, type_name):
    tn = type_name.lower()
    if tn in ("int", "integer", "long") and isinstance(val, int) and not isinstance(val, bool):
        return True
    if tn in ("float", "double", "real", "number") and isinstance(val, float):
        return True
    if tn in ("str", "string", "text") and isinstance(val, str):
        return True
    if tn in ("bool", "boolean") and isinstance(val, bool):
        return True
    if tn in ("none", "null", "nil", "void") and val is None:
        return True
    if tn in ("list", "array", "vec") and isinstance(val, list):
        return True
    if tn in ("dict", "map", "object") and isinstance(val, dict):
        return True
    if tn in ("tuple",) and isinstance(val, tuple):
        return True
    if tn in ("function", "fn", "callable") and isinstance(val, (Function, NativeFunction, BoundBuiltin)):
        return True
    if tn in ("channel", "chan") and isinstance(val, Channel):
        return True
    if tn in ("generator", "iter") and isinstance(val, Generator):
        return True
    if isinstance(val, Instance) and val.cls.name == type_name:
        return True
    return False


def display(v):
    """How a value is shown by PRINT (strings without quotes)."""
    if v is None: return "none"
    if isinstance(v, bool): return "true" if v else "false"
    if isinstance(v, str): return v
    if isinstance(v, float):
        if v == int(v) and abs(v) < 1e16:
            return f"{v:.1f}"
        return repr(v)
    if isinstance(v, int):
        return str(v)
    if isinstance(v, list):
        return "[" + ", ".join(repr_val(x) for x in v) + "]"
    if isinstance(v, tuple):
        return "(" + ", ".join(repr_val(x) for x in v) + (")" if len(v) != 1 else ",)")
    if isinstance(v, dict):
        return "{" + ", ".join(f"{repr_val(k)}: {repr_val(val)}" for k, val in v.items()) + "}"
    if isinstance(v, range):
        return f"range({v.start}, {v.stop})"
    if isinstance(v, Function):
        return f"<function {v.name or 'anonymous'}>"
    if isinstance(v, NativeFunction):
        return f"<native {v.name}>"
    if isinstance(v, ClassObj):
        return f"<class {v.name}>"
    if isinstance(v, Instance):
        return f"<{v.cls.name}>"
    if isinstance(v, Channel):
        return f"<channel>"
    if isinstance(v, Generator):
        return "<generator>"
    if isinstance(v, Future):
        return f"<future {'done' if v.done else 'pending'}>"
    if isinstance(v, _EnumNamespace):
        return f"<enum {v.name}>"
    if isinstance(v, _re.Pattern):
        return f"re\"{v.pattern}\""
    return str(v)


def repr_val(v):
    if isinstance(v, str): return '"' + v.replace("\\", "\\\\").replace('"', '\\"') + '"'
    return display(v)


def _parse_input_value(line):
    s = line.strip()
    if s == "":
        return ""
    # try int, float, else string
    try:
        return int(s)
    except ValueError:
        pass
    try:
        return float(s)
    except ValueError:
        pass
    return s


def _format_using(fmt, vals):
    r"""A tiny PRINT USING implementation: # = digit, . = decimal point,
    $ = currency, , = thousands, \ = string char, & = whole string."""
    out = []
    vi = 0
    i = 0
    n = len(fmt)
    while i < n:
        c = fmt[i]
        if c == "#":
            # gather digit group
            j = i
            while j < n and fmt[j] in "#.,$+-":
                j += 1
            spec = fmt[i:j]
            v = vals[vi] if vi < len(vals) else 0; vi += 1
            out.append(_format_number(spec, v))
            i = j
            continue
        if c == "\\":
            j = i
            while j < n and fmt[j] == "\\":
                j += 1
            if j < n and fmt[j] == " ":
                j += 1
            width = j - i
            v = display(vals[vi]) if vi < len(vals) else ""; vi += 1
            out.append(v[:width].ljust(width))
            i = j
            continue
        if c == "&":
            v = display(vals[vi]) if vi < len(vals) else ""; vi += 1
            out.append(v); i += 1; continue
        out.append(c); i += 1
    return "".join(out)


def _format_number(spec, v):
    import re
    if isinstance(v, str):
        try: v = float(v)
        except ValueError: v = 0
    if isinstance(v, bool): v = 1 if v else 0
    if v is None: v = 0
    has_dot = "." in spec
    decimals = 0
    if has_dot:
        decimals = len(spec) - spec.index(".") - 1
    neg = v < 0
    av = abs(v)
    s = f"{av:.{decimals}f}"
    if "$" in spec:
        s = "$" + s
    if "," in spec:
        # thousands separators
        intpart, _, frac = s.partition(".")
        intpart = f"{int(intpart.replace('$','')):,}"
        if "$" in spec: intpart = "$" + intpart
        s = intpart + (("." + frac) if frac else "")
    if "+" in spec:
        s = ("+" if not neg else "-") + s
    elif neg:
        s = "-" + s
    return s


def _apply_fmt(s, v, fmt):
    fmt = fmt.strip()
    if not fmt:
        return s
    if fmt.endswith("d") or fmt.endswith("i"):
        try:
            return format(int(v), fmt)
        except Exception:
            return s
    if fmt.endswith(("f", "e", "g", "E", "G")):
        try:
            return format(float(v), fmt)
        except Exception:
            return s
    if fmt.endswith("x") or fmt.endswith("X") or fmt.endswith("o") or fmt.endswith("b"):
        try:
            return format(int(v), fmt)
        except Exception:
            return s
    if fmt.endswith("s"):
        try:
            return format(s, fmt)
        except Exception:
            return s
    try:
        return format(v, fmt)
    except Exception:
        return s


# ---- dispatch tables -----------------------------------------------------
Interpreter._stmt_dispatch = {
    "exprstmt": Interpreter.s_exprstmt,
    "label": Interpreter.s_label,
    "pass": Interpreter.s_pass,
    "stop": Interpreter.s_stop,
    "print": Interpreter.s_print,
    "input": Interpreter.s_input,
    "let": Interpreter.s_let,
    "destructure": Interpreter.s_destructure,
    "dim": Interpreter.s_dim,
    "block": Interpreter.s_block,
    "if": Interpreter.s_if,
    "for": Interpreter.s_for,
    "forin": Interpreter.s_forin,
    "while": Interpreter.s_while,
    "repeat": Interpreter.s_repeat,
    "select": Interpreter.s_select,
    "match": Interpreter.s_match,
    "func": Interpreter.s_func,
    "sub": Interpreter.s_sub,
    "return": Interpreter.s_return,
    "break": Interpreter.s_break,
    "continue": Interpreter.s_continue,
    "yield": Interpreter.s_yield,
    "throw": Interpreter.s_throw,
    "try": Interpreter.s_try,
    "class": Interpreter.s_class,
    "enum": Interpreter.s_enum,
    "import": Interpreter.s_import,
    "export": Interpreter.s_export,
    "goto": Interpreter.s_goto,
    "gosub": Interpreter.s_gosub,
    "data": Interpreter.s_data,
    "read": Interpreter.s_read,
    "restore": Interpreter.s_restore,
    "randomize": Interpreter.s_randomize,
    "optionbase": Interpreter.s_optionbase,
    "defer": Interpreter.s_defer,
    "spawn": Interpreter.s_spawn,
    "send": Interpreter.s_send,
    "with": Interpreter.s_with,
    "assign": Interpreter.s_assign,
    "assignop": Interpreter.s_assignop,
}

Interpreter._expr_dispatch = {
    "int": Interpreter.e_int,
    "float": Interpreter.e_float,
    "str": Interpreter.e_str,
    "bool": Interpreter.e_bool,
    "none": Interpreter.e_none,
    "regex": Interpreter.e_regex,
    "fstring": Interpreter.e_fstring,
    "ident": Interpreter.e_ident,
    "list": Interpreter.e_list,
    "dict": Interpreter.e_dict,
    "tuple": Interpreter.e_tuple,
    "range": Interpreter.e_range,
    "comprehension": Interpreter.e_comprehension,
    "binop": Interpreter.e_binop,
    "unop": Interpreter.e_unop,
    "logical": Interpreter.e_logical,
    "assign": Interpreter.e_assign,
    "assignop": Interpreter.e_assignop,
    "call": Interpreter.e_call,
    "index": Interpreter.e_index,
    "slice": Interpreter.e_slice,
    "member": Interpreter.e_member,
    "optmember": Interpreter.e_optmember,
    "lambda": Interpreter.e_lambda,
    "ternary": Interpreter.e_ternary,
    "nullish": Interpreter.e_nullish,
    "await": Interpreter.e_await,
    "chanrecv": Interpreter.e_chanrecv,
    "matchexpr": Interpreter.e_matchexpr,
    "spawn": Interpreter.e_spawn,
    "send": Interpreter.e_send,
    "spread": Interpreter.e_spread,
}
