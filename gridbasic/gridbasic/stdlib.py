"""The GridBasic standard library — built-in functions available to every
program without an import. Each function receives ``(args, kwargs)`` where
``args`` is a list of already-evaluated GridBasic values and ``kwargs`` is a
dict, and returns a GridBasic value (or raises :class:`GBRuntimeError`).
"""

from __future__ import annotations

import math
import random
import time as _time
import json as _json
import os as _os
import sys as _sys
from .errors import GBRuntimeError, ThrowSignal


def _pyify(v):
    """Recursively convert GridBasic runtime values into JSON-safe Python."""
    if isinstance(v, tuple): return [_pyify(x) for x in v]
    if isinstance(v, list): return [_pyify(x) for x in v]
    if isinstance(v, dict): return {k: _pyify(x) for k, x in v.items()}
    try:
        from .interpreter import Instance
        if isinstance(v, Instance):
            return {k: _pyify(x) for k, x in v.fields.items()}
    except Exception:
        pass
    return v


def functions(interp):
    """Return the dict of builtin name -> callable exposed in the global env."""
    # Lazy import to avoid a circular dependency at module load time.
    from .interpreter import display, repr_val, _truthy
    g = {}

    def reg(name, arity=None):
        def deco(fn):
            g[name] = fn
            return fn
        return deco

    # ---- I/O ----------------------------------------------------------
    def _print(args, kwargs):
        sep = kwargs.get("sep", " ")
        end = kwargs.get("end", "\n")
        if not isinstance(sep, str): sep = display(sep)
        if not isinstance(end, str): end = display(end)
        out = sep.join(display(a) for a in args)
        interp.emit(out + end)
        return None
    g["print"] = _print
    g["println"] = _print

    def _say(args, kwargs):
        # alias that uses the AI voice if available (otherwise plain print)
        return _print(args, kwargs)
    g["say"] = _say

    def _input(args, kwargs):
        prompt = display(args[0]) if args else ""
        return interp.read_input(prompt)
    g["input"] = _input

    def _readln(args, kwargs):
        return interp.read_input("")
    g["readln"] = _readln
    g["readline"] = _readln

    # ---- type constructors / conversions ------------------------------
    def _int(args, kwargs):
        if not args: return 0
        v = args[0]
        if isinstance(v, bool): return 1 if v else 0
        if isinstance(v, int): return v
        if isinstance(v, float): return int(v)
        if isinstance(v, str):
            try: return int(v.strip())
            except ValueError:
                try: return int(float(v.strip()))
                except ValueError:
                    raise GBRuntimeError(f"Cannot convert {v!r} to int")
        if v is None: return 0
        raise GBRuntimeError(f"Cannot convert {display(v)} to int")
    g["int"] = _int

    def _float(args, kwargs):
        if not args: return 0.0
        v = args[0]
        if isinstance(v, bool): return 1.0 if v else 0.0
        if isinstance(v, (int, float)): return float(v)
        if isinstance(v, str):
            try: return float(v.strip())
            except ValueError:
                raise GBRuntimeError(f"Cannot convert {v!r} to float")
        if v is None: return 0.0
        raise GBRuntimeError(f"Cannot convert {display(v)} to float")
    g["float"] = _float

    def _str(args, kwargs):
        return display(args[0]) if args else ""
    g["str"] = _str
    g["string"] = _str

    def _bool(args, kwargs):
        return _truthy(args[0]) if args else False
    g["bool"] = _bool

    def _num(args, kwargs):
        v = args[0] if args else 0
        if isinstance(v, (int, float)): return v
        return _float([v], {})
    g["num"] = _num

    def _list(args, kwargs):
        if not args: return []
        v = args[0]
        if isinstance(v, (list, tuple, range)): return list(v)
        if isinstance(v, str): return list(v)
        if isinstance(v, dict): return list(v.keys())
        if v is None: return []
        return [v]
    g["list"] = _list
    g["array"] = _list

    def _dict(args, kwargs):
        if not args: return {}
        v = args[0]
        if isinstance(v, dict): return dict(v)
        if isinstance(v, list):
            d = {}
            for item in v:
                if isinstance(item, (list, tuple)) and len(item) == 2:
                    d[item[0]] = item[1]
            return d
        return {}
    g["dict"] = _dict
    g["map"] = _dict

    def _tuple(args, kwargs):
        return tuple(args)
    g["tuple"] = _tuple

    def _set(args, kwargs):
        out = []
        for a in args:
            if isinstance(a, (list, tuple, range)):
                for x in a:
                    if x not in out: out.append(x)
            elif a not in out:
                out.append(a)
        return out
    g["set"] = _set

    # ---- type queries -------------------------------------------------
    def _type(args, kwargs):
        v = args[0] if args else None
        if v is None: return "none"
        if isinstance(v, bool): return "bool"
        if isinstance(v, int): return "int"
        if isinstance(v, float): return "float"
        if isinstance(v, str): return "string"
        if isinstance(v, list): return "list"
        if isinstance(v, tuple): return "tuple"
        if isinstance(v, dict): return "dict"
        if isinstance(v, range): return "range"
        from .interpreter import Function, NativeFunction, ClassObj, Instance, Channel, Generator, Future
        if isinstance(v, Function): return "function"
        if isinstance(v, NativeFunction): return "function"
        if isinstance(v, ClassObj): return "class"
        if isinstance(v, Instance): return v.cls.name
        if isinstance(v, Channel): return "channel"
        if isinstance(v, Generator): return "generator"
        if isinstance(v, Future): return "future"
        return type(v).__name__
    g["type"] = _type
    g["typeof"] = _type

    def _is_none(args, kwargs): return args[0] is None
    g["is_none"] = _is_none
    def _is_int(args, kwargs):
        v = args[0]; return isinstance(v, int) and not isinstance(v, bool)
    g["is_int"] = _is_int
    def _is_str(args, kwargs): return isinstance(args[0], str)
    g["is_str"] = _is_str
    def _is_list(args, kwargs): return isinstance(args[0], list)
    g["is_list"] = _is_list
    def _is_dict(args, kwargs): return isinstance(args[0], dict)
    g["is_dict"] = _is_dict
    def _is_callable(args, kwargs):
        from .interpreter import callable_value
        return callable_value(args[0])
    g["is_callable"] = _is_callable

    def _len(args, kwargs):
        v = args[0]
        try: return len(v)
        except TypeError:
            raise GBRuntimeError(f"len() needs a sized value, got {display(v)}")
    g["len"] = _len
    g["size"] = _len

    def _range(args, kwargs):
        if not args: return range(0)
        if len(args) == 1: return range(int(args[0]))
        if len(args) == 2: return range(int(args[0]), int(args[1]))
        return range(int(args[0]), int(args[1]), int(args[2]))
    g["range"] = _range

    def _repr(args, kwargs): return repr_val(args[0])
    g["repr"] = _repr

    def _hash(args, kwargs):
        try: return hash(args[0])
        except TypeError: return hash(display(args[0]))
    g["hash"] = _hash

    def _id(args, kwargs): return id(args[0])
    g["id"] = _id

    # ---- math ---------------------------------------------------------
    def _abs(args, kwargs): return abs(args[0])
    g["abs"] = _abs
    def _min(args, kwargs):
        if len(args) == 1 and isinstance(args[0], (list, tuple)):
            return min(args[0]) if args[0] else None
        return min(args) if args else None
    g["min"] = _min
    def _max(args, kwargs):
        if len(args) == 1 and isinstance(args[0], (list, tuple)):
            return max(args[0]) if args[0] else None
        return max(args) if args else None
    g["max"] = _max
    def _round(args, kwargs):
        v = args[0]
        nd = int(args[1]) if len(args) > 1 else 0
        return round(v, nd)
    g["round"] = _round
    def _sqrt(args, kwargs): return math.sqrt(args[0])
    g["sqrt"] = _sqrt
    def _pow(args, kwargs): return args[0] ** args[1] if len(args) > 1 else math.pow(args[0], args[0])
    g["pow"] = _pow
    def _floor(args, kwargs): return math.floor(args[0])
    g["floor"] = _floor
    def _ceil(args, kwargs): return math.ceil(args[0])
    g["ceil"] = _ceil
    def _sin(args, kwargs): return math.sin(args[0])
    g["sin"] = _sin
    def _cos(args, kwargs): return math.cos(args[0])
    g["cos"] = _cos
    def _tan(args, kwargs): return math.tan(args[0])
    g["tan"] = _tan
    def _log(args, kwargs):
        if len(args) > 1: return math.log(args[0], args[1])
        return math.log(args[0])
    g["log"] = _log
    def _log2(args, kwargs): return math.log2(args[0])
    g["log2"] = _log2
    def _log10(args, kwargs): return math.log10(args[0])
    g["log10"] = _log10
    def _exp(args, kwargs): return math.exp(args[0])
    g["exp"] = _exp
    def _pi(args, kwargs): return math.pi
    g["pi"] = _pi
    def _e(args, kwargs): return math.e
    g["e"] = _e
    def _gcd(args, kwargs): return math.gcd(int(args[0]), int(args[1]))
    g["gcd"] = _gcd
    def _sign(args, kwargs):
        v = args[0]
        return (v > 0) - (v < 0)
    g["sign"] = _sign
    def _clamp(args, kwargs):
        v, lo, hi = args[0], args[1], args[2]
        return max(lo, min(hi, v))
    g["clamp"] = _clamp

    # ---- random -------------------------------------------------------
    def _random(args, kwargs):
        if not args: return random.random()
        if len(args) == 1: return random.uniform(0, args[0])
        return random.uniform(args[0], args[1])
    g["random"] = _random
    g["rnd"] = _random
    def _randint(args, kwargs): return random.randint(int(args[0]), int(args[1]))
    g["randint"] = _randint
    def _choice(args, kwargs): return random.choice(args[0]) if args[0] else None
    g["choice"] = _choice
    def _shuffle(args, kwargs):
        v = list(args[0]); random.shuffle(v); return v
    g["shuffle"] = _shuffle
    def _sample(args, kwargs):
        return random.sample(list(args[0]), int(args[1]))
    g["sample"] = _sample

    # ---- string helpers ----------------------------------------------
    def _chr(args, kwargs): return chr(int(args[0]))
    g["chr"] = _chr
    def _ord(args, kwargs): return ord(args[0])
    g["ord"] = _ord
    def _hex(args, kwargs): return hex(int(args[0]))
    g["hex"] = _hex
    def _bin(args, kwargs): return bin(int(args[0]))
    g["bin"] = _bin
    def _oct(args, kwargs): return oct(int(args[0]))
    g["oct"] = _oct
    def _format(args, kwargs):
        fmt = args[0]
        rest = args[1:]
        try:
            return fmt.format(*rest)
        except Exception:
            return fmt
    g["format"] = _format

    def _split(args, kwargs):
        s = args[0]; sep = args[1] if len(args) > 1 else None
        return s.split(sep) if sep is not None else s.split()
    g["split"] = _split
    def _join(args, kwargs):
        lst = args[0]; sep = args[1] if len(args) > 1 else ""
        return sep.join(display(x) for x in lst)
    g["join"] = _join
    def _replace(args, kwargs): return args[0].replace(args[1], args[2])
    g["replace"] = _replace
    def _trim(args, kwargs): return args[0].strip()
    g["trim"] = _trim
    def _upper(args, kwargs): return args[0].upper()
    g["upper"] = _upper
    def _lower(args, kwargs): return args[0].lower()
    g["lower"] = _lower
    def _contains(args, kwargs): return args[1] in args[0]
    g["contains"] = _contains
    def _reverse(args, kwargs):
        v = args[0]
        if isinstance(v, str): return v[::-1]
        if isinstance(v, list): return v[::-1]
        raise GBRuntimeError("reverse() needs a string or list")
    g["reverse"] = _reverse

    def _left(args, kwargs):
        s = display(args[0]); n = int(args[1]) if len(args) > 1 else 1
        return s[:n]
    g["left"] = _left
    def _right(args, kwargs):
        s = display(args[0]); n = int(args[1]) if len(args) > 1 else 1
        return s[-n:] if n else ""
    g["right"] = _right
    def _mid(args, kwargs):
        s = display(args[0]); start = int(args[1])
        n = int(args[2]) if len(args) > 2 else len(s)
        return s[start-1:start-1+n]
    g["mid"] = _mid

    def _matches(args, kwargs):
        import re
        s, pat = args[0], args[1]
        m = re.search(pat, s)
        return m is not None
    g["matches"] = _matches

    def _regex(args, kwargs):
        import re
        return re.compile(args[0])
    g["regex"] = _regex

    # ---- collections --------------------------------------------------
    def _sorted(args, kwargs):
        v = args[0]
        return sorted(v, key=lambda x: x)
    g["sorted"] = _sorted
    def _reversed(args, kwargs): return list(reversed(args[0]))
    g["reversed"] = _reversed
    def _sum(args, kwargs):
        v = args[0] if args else []
        return sum(x for x in v if isinstance(x, (int, float)) and not isinstance(x, bool))
    g["sum"] = _sum
    def _enumerate(args, kwargs):
        return [[i, x] for i, x in enumerate(args[0])]
    g["enumerate"] = _enumerate
    def _zip(args, kwargs):
        return [list(t) for t in zip(*args)]
    g["zip"] = _zip
    def _any(args, kwargs): return any(_truthy(x) for x in args[0])
    g["any"] = _any
    def _all(args, kwargs): return all(_truthy(x) for x in args[0])
    g["all"] = _all
    def _flat(args, kwargs):
        out = []
        for x in args[0]:
            if isinstance(x, list): out.extend(x)
            else: out.append(x)
        return out
    g["flat"] = _flat
    g["flatten"] = _flat

    def _push(args, kwargs):
        args[0].append(args[1]); return args[0]
    g["push"] = _push

    def _keys(args, kwargs): return list(args[0].keys())
    g["keys"] = _keys
    def _values(args, kwargs): return list(args[0].values())
    g["values"] = _values
    def _items(args, kwargs): return [list(t) for t in args[0].items()]
    g["items"] = _items

    # ---- time ---------------------------------------------------------
    def _time_fn(args, kwargs): return _time.time()
    g["time"] = _time_fn
    g["tick"] = _time_fn
    def _now(args, kwargs): return _time.strftime("%Y-%m-%d %H:%M:%S")
    g["now"] = _now
    def _sleep(args, kwargs):
        _time.sleep(float(args[0])); return None
    g["sleep"] = _sleep
    def _delay(args, kwargs):
        _time.sleep(float(args[0]) / 1000.0); return None
    g["delay"] = _delay
    def _clock(args, kwargs): return _time.perf_counter()
    g["clock"] = _clock

    # ---- filesystem ---------------------------------------------------
    def _read_file(args, kwargs):
        path = args[0]
        try:
            with open(path, "r", encoding="utf-8") as fh:
                return fh.read()
        except Exception as e:
            raise GBRuntimeError(f"read_file error: {e}")
    g["read_file"] = _read_file
    g["readfile"] = _read_file

    def _write_file(args, kwargs):
        path, content = args[0], args[1]
        try:
            with open(path, "w", encoding="utf-8") as fh:
                fh.write(content)
            return True
        except Exception as e:
            raise GBRuntimeError(f"write_file error: {e}")
    g["write_file"] = _write_file
    g["writefile"] = _write_file

    def _append_file(args, kwargs):
        path, content = args[0], args[1]
        with open(path, "a", encoding="utf-8") as fh:
            fh.write(content)
        return True
    g["append_file"] = _append_file

    def _exists(args, kwargs): return _os.path.exists(args[0])
    g["exists"] = _exists
    def _list_dir(args, kwargs): return _os.listdir(args[0])
    g["list_dir"] = _list_dir
    g["listdir"] = _list_dir

    def _shell(args, kwargs):
        import subprocess
        cmd = args[0]
        if isinstance(cmd, str):
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        else:
            result = subprocess.run(cmd, capture_output=True, text=True)
        return {"exit": result.returncode, "out": result.stdout, "err": result.stderr}
    g["shell"] = _shell

    # ---- JSON ---------------------------------------------------------
    def _to_json(args, kwargs):
        return _json.dumps(_pyify(args[0]))
    g["to_json"] = _to_json
    def _from_json(args, kwargs):
        try: return _json.loads(args[0])
        except Exception as e:
            raise GBRuntimeError(f"from_json error: {e}")
    g["from_json"] = g["json_parse"] = _from_json

    # ---- control / errors --------------------------------------------
    def _error(args, kwargs):
        msg = display(args[0]) if args else "error"
        raise ThrowSignal(msg)
    g["error"] = _error
    g["throw"] = _error
    g["raise"] = _error

    def _panic(args, kwargs):
        msg = display(args[0]) if args else "panic"
        interp.emit("PANIC: " + msg + "\n")
        raise GBRuntimeError(msg)

    def _assert(args, kwargs):
        cond = args[0] if args else False
        if not _truthy(cond):
            msg = display(args[1]) if len(args) > 1 else "assertion failed"
            raise ThrowSignal(msg)
        return None
    g["assert"] = _assert

    def _exit(args, kwargs):
        from .interpreter import StopSignal
        raise StopSignal()
    g["exit"] = _exit

    def _try(args, kwargs):
        # try(fn, catch_fn) -> convenience
        fn = args[0]
        catch = args[1] if len(args) > 1 else None
        try:
            return interp.call_value(fn, [], {})
        except ThrowSignal as t:
            if catch is not None:
                return interp.call_value(catch, [t.value], {})
            return None
    g["try"] = _try

    # ---- introspection / eval ----------------------------------------
    def _eval(args, kwargs):
        from .lexer import Lexer
        from .parser import Parser
        src = args[0]
        prog = Parser(Lexer(src).tokenize()).parse()
        # eval as expression if single expr stmt, else execute
        if len(prog.body) == 1 and prog.body[0].kind == "exprstmt":
            return interp.eval(prog.body[0].expr)
        for s in prog.body:
            interp.exec_stmt(s)
        return None
    g["eval"] = _eval

    def _apply(args, kwargs):
        fn = args[0]
        arglist = args[1] if len(args) > 1 else []
        return interp.call_value(fn, list(arglist), dict(kwargs))
    g["apply"] = _apply

    def _help(args, kwargs):
        topic = args[0] if args else None
        interp.emit(_help_text(topic))
        return None
    g["help"] = _help

    def _version(args, kwargs):
        from . import __version__
        return f"GridBasic {__version__}"
    g["version"] = _version

    # ---- crypto / irc / ai shortcuts (delegated to modules) ----------
    # These are also available as IRC.X / CRYPTO.X / AI.X. The upper-case
    # module objects are placed in globals by the interpreter; here we add
    # a couple of convenience top-levels.

    return g


def _help_text(topic):
    if topic is None:
        return ("GridBasic — the most advanced BASIC on Earth.\n"
                "Modules: IRC, CRYPTO, AI, GRID, MATH, STRING, TIME, JSON, IO\n"
                "Type help(\"irc\"), help(\"crypto\"), help(\"ai\") for details.\n")
    t = str(topic).lower()
    texts = {
        "irc": "IRC.connect(host, port, nick) -> connection. conn.join('#chan'), conn.send('#chan', 'hi'), conn.on_message(fn), conn.loop().",
        "crypto": "CRYPTO.wallet() -> keypair. CRYPTO.address(w) -> addr. CRYPTO.sign(w, msg) -> sig. CRYPTO.verify(pub, msg, sig). CRYPTO.mine(block, difficulty).",
        "ai": "AI.generate(prompt, model='markov') -> text. AI.embed(text). AI.train(text). AI.model('perceptron'). AI.api(key, prompt).",
        "grid": "GRID.gui(), GRID.plot(x, y), GRID.color(c), GRID.clear(), GRID.http_get(url), GRID.http_post(url, body).",
    }
    return texts.get(t, f"No help for {topic}")


def math_namespace():
    return {
        "pi": math.pi, "e": math.e, "tau": math.tau, "inf": math.inf, "nan": float("nan"),
        "sqrt": math.sqrt, "sin": math.sin, "cos": math.cos, "tan": math.tan,
        "asin": math.asin, "acos": math.acos, "atan": math.atan, "atan2": math.atan2,
        "log": math.log, "log2": math.log2, "log10": math.log10, "exp": math.exp,
        "floor": math.floor, "ceil": math.ceil, "trunc": math.trunc,
        "pow": math.pow, "hypot": math.hypot, "gcd": math.gcd, "lcm": math.lcm,
        "degrees": math.degrees, "radians": math.radians, "fabs": math.fabs,
        "factorial": math.factorial, "isfinite": math.isfinite, "isnan": math.isnan,
        "sinh": math.sinh, "cosh": math.cosh, "tanh": math.tanh,
    }


def string_namespace():
    return {
        "upper": lambda s: s.upper(), "lower": lambda s: s.lower(),
        "trim": lambda s: s.strip(), "ltrim": lambda s: s.lstrip(),
        "rtrim": lambda s: s.rstrip(), "reverse": lambda s: s[::-1],
        "repeat": lambda s, n: s * n, "contains": lambda s, x: x in s,
        "starts_with": lambda s, x: s.startswith(x),
        "ends_with": lambda s, x: s.endswith(x),
        "split": lambda s, sep=None: s.split(sep) if sep else s.split(),
        "join": lambda parts, sep="": sep.join(parts),
        "replace": lambda s, a, b: s.replace(a, b),
        "length": lambda s: len(s), "find": lambda s, x: s.find(x),
        "substr": lambda s, a, b=None: s[a:b] if b is not None else s[a:],
        "format": lambda fmt, *a: fmt.format(*a),
    }


def time_namespace():
    return {
        "now": _time.time, "clock": _time.perf_counter,
        "sleep": _time.sleep, "strftime": _time.strftime,
        "strptime": _time.strptime, "time": _time.time,
        "monotonic": _time.monotonic,
        "date": lambda: _time.strftime("%Y-%m-%d"),
        "datetime": lambda: _time.strftime("%Y-%m-%d %H:%M:%S"),
    }


def json_namespace():
    return {
        "parse": _json.loads,
        "stringify": lambda v, *a: _json.dumps(_pyify(v), *a),
        "dump": lambda v, path: open(path, "w").write(_json.dumps(_pyify(v))),
        "load": lambda path: _json.loads(open(path).read()),
    }


def io_namespace():
    return {
        "read": _os.path.exists,
        "read_file": lambda p: open(p, "r", encoding="utf-8").read(),
        "write_file": lambda p, c: open(p, "w", encoding="utf-8").write(c) or True,
        "append_file": lambda p, c: open(p, "a", encoding="utf-8").write(c) or True,
        "exists": _os.path.exists,
        "list_dir": _os.listdir,
        "mkdir": _os.makedirs,
        "remove": _os.remove,
    }
