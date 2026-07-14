"""Error types for the GridBasic interpreter.

All GridBasic errors derive from :class:`GridBasicError` so callers can catch
the whole family with one ``except``. Each error carries a line number when
available so the IDE can underline the offending source.
"""


class GridBasicError(Exception):
    """Base class for every GridBasic error."""

    def __init__(self, message: str, line: int = 0, col: int = 0):
        super().__init__(message)
        self.message = message
        self.line = line
        self.col = col

    def __str__(self) -> str:
        if self.line:
            return f"[line {self.line}] {self.message}"
        return self.message


class LexError(GridBasicError):
    """Raised by the lexer for unterminated strings, bad numbers, etc."""


class ParseError(GridBasicError):
    """Raised by the parser for unexpected tokens / malformed syntax."""


class GBRuntimeError(GridBasicError):
    """Raised by the interpreter during execution."""


class ReturnSignal(Exception):
    """Internal control-flow signal raised by ``RETURN`` statements."""

    def __init__(self, value):
        super().__init__("return")
        self.value = value


class BreakSignal(Exception):
    """Internal control-flow signal raised by ``BREAK``/``EXIT FOR``."""


class ContinueSignal(Exception):
    """Internal control-flow signal raised by ``CONTINUE``."""


class ThrowSignal(Exception):
    """Internal control-flow signal raised by ``THROW`` (carries a GridBasic value)."""

    def __init__(self, value):
        super().__init__("throw")
        self.value = value


class YieldSignal(Exception):
    """Internal control-flow signal raised by ``YIELD`` in generator functions."""

    def __init__(self, value):
        super().__init__("yield")
        self.value = value
