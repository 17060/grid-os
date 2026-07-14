#!/usr/bin/env python3
"""GridBasic command-line runner.

Usage::

    python -m gridbasic              # REPL
    python -m gridbasic program.gb   # run a file
    python -m gridbasic -c "PRINT 2+2"
    python -m gridbasic --ide         # launch the GUI IDE server
"""

from __future__ import annotations

import sys
import os

# When run as a script, make the package importable.
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, os.path.dirname(_HERE))


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    ide_mode = False
    open_browser = True
    cmd = None
    files = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ("--ide", "--gui", "-i"):
            ide_mode = True
        elif a in ("--no-browser", "--headless"):
            open_browser = False
        elif a in ("--port",):
            i += 1  # consumed by serve() via env later
        elif a in ("--version", "-v"):
            from . import version
            print(version())
            return 0
        elif a in ("--help", "-h"):
            print(__doc__)
            return 0
        elif a == "-c":
            cmd = argv[i + 1]; i += 1
        else:
            files.append(a)
        i += 1

    if ide_mode:
        from .ide.server import serve
        return serve(open_browser=open_browser)

    if cmd is not None:
        from . import run
        try:
            run(cmd)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return 1
        return 0

    if files:
        from . import run_file
        for path in files:
            try:
                run_file(path)
            except Exception as e:
                print(f"Error in {path}: {e}", file=sys.stderr)
                return 1
        return 0

    # REPL
    return _repl()


def _repl():
    from . import version
    from .interpreter import Interpreter
    from .errors import GridBasicError
    print(version() + " — REPL. Type 'exit' to quit.")
    interp = Interpreter()
    buffer = ""
    while True:
        try:
            prompt = "gb> " if not buffer else "... "
            line = input(prompt)
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if line.strip() == "exit" or line.strip() == "quit":
            break
        buffer += line + "\n"
        # try to run; if incomplete, keep buffering on a multi-line keyword
        if _is_complete(buffer):
            try:
                interp.run(buffer)
            except GridBasicError as e:
                print(f"Error: {e}")
            except Exception as e:
                print(f"Internal error: {e}")
            buffer = ""
    return 0


def _is_complete(src):
    # crude: complete if last line doesn't end with a block-opening keyword
    # and parens/brackets are balanced
    for opener, closer in (("(", ")"), ("[", "]"), ("{", "}")):
        if src.count(opener) != src.count(closer):
            return False
    stripped = src.rstrip()
    if not stripped:
        return True
    last = stripped.splitlines()[-1].strip().lower()
    # Don't try to run while the user just opened a block.
    openers = ("function", "sub", "def", "if", "for", "while", "do", "repeat",
               "select", "match", "try", "class", "enum", "fn", "else", "elseif",
               "case", "catch", "finally")
    for op in openers:
        if last == op or last.startswith(op + " ") and "then" not in last:
            # only block-open if it doesn't end the statement
            if last in openers or (last.startswith(op) and not _line_closes(last, op)):
                return False
    return True


def _line_closes(line, op):
    if op == "if" and "then" in line:
        # single-line IF: complete only if it ends with a statement after THEN
        after = line.split("then", 1)[1].strip()
        return bool(after)
    return False


if __name__ == "__main__":
    sys.exit(main())
