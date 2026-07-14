"""GridBasic — the most advanced BASIC dialect on Earth.

GridBasic is a modern, multi-paradigm programming language that keeps the
approachable, line-oriented spirit of classic BASIC while integrating the
best ideas from across the universe of programming languages:

  * Python   — list/dict comprehensions, f-strings, generators, decorators,
               first-class functions, dynamic typing with optional hints.
  * JavaScript/TypeScript — async/await, arrow functions, destructuring,
               optional chaining (`?.`), nullish coalescing (`??`).
  * Rust     — `match` pattern matching, `Result`/`Option`, immutable-by-default `let`.
  * Go       — lightweight concurrency (`spawn`), channels (`chan`, `<-`), `defer`.
  * Haskell/ML — pattern matching, tail-friendly recursion, higher-order functions.
  * Ruby     — blocks/iterators, mixin traits.
  * Swift/Kotlin — optionals, smart flow, `enum`, `trait`/`impl`.
  * Lisp     — functions as values, homoiconic-ish macros (compile-time `#`).
  * SQL      — comprehension queries that read like queries.
  * Classic BASIC — `PRINT`, `INPUT`, `FOR/NEXT`, `GOTO/GOSUB`, line numbers (optional).

It ships with three flagship built-in capabilities exposed as language modules:
  * ``IRC``   — connect to IRC networks, join channels, send/receive messages.
  * ``CRYPTO``— keypairs, addresses, signed transactions, a hash-linked ledger, mining.
  * ``AI``    — run local models (n-gram, markov, perceptron, tiny transformer)
                 and bridge to external LLM APIs.

This package exposes the interpreter and a small public API.
"""

from .errors import GridBasicError, LexError, ParseError, GBRuntimeError
from .lexer import Lexer, Token, TokenType
from .parser import Parser
from .interpreter import Interpreter
from . import stdlib

__version__ = "1.0.0"
__language__ = "GridBasic 1.0"

__all__ = [
    "GridBasicError", "LexError", "ParseError", "GBRuntimeError",
    "Lexer", "Token", "TokenType",
    "Parser", "Interpreter", "stdlib",
    "run", "run_file", "version",
]


def version() -> str:
    return f"GridBasic {__version__}"


def run(source: str, *, output=None, stdin=None, max_steps: int = 5_000_000):
    """Run a GridBasic source string and return the Interpreter instance."""
    interp = Interpreter(output=output, stdin=stdin, max_steps=max_steps)
    interp.run(source)
    return interp


def run_file(path: str, *, output=None, stdin=None, max_steps: int = 5_000_000):
    with open(path, "r", encoding="utf-8") as fh:
        return run(fh.read(), output=output, stdin=stdin, max_steps=max_steps)
