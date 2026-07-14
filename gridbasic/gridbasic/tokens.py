"""Token definitions for the GridBasic lexer."""

from enum import Enum, auto


class TokenType(Enum):
    # Literals
    INT = auto()
    FLOAT = auto()
    STRING = auto()
    FSTRING = auto()       # f"..." or $"..." interpolated string
    REGEX = auto()

    # Identifiers and keywords
    IDENT = auto()
    KEYWORD = auto()

    # Operators
    PLUS = auto()          # +
    MINUS = auto()         # -
    STAR = auto()          # *
    SLASH = auto()         # /
    PERCENT = auto()       # %
    CARET = auto()         # ^  (power)
    DOT = auto()           # .
    DOTDOT = auto()        # ..   (range / slice)
    DOTDOTDOT = auto()     # ...  (spread)
    DOTDOTEQ = auto()      # ..=  (inclusive range)

    # Comparison / assignment
    EQ = auto()            # =
    EQEQ = auto()          # ==
    NEQ = auto()           # !=  and <>
    LT = auto()            # <
    GT = auto()            # >
    LE = auto()            # <=
    GE = auto()            # >=
    COLONEQ = auto()       # :=  (walrus / reassign)
    ARROW = auto()         # =>  (lambda / match arm)
    SEND = auto()          # <-  (channel send / Go-style)
    COLONCOLON = auto()    # ::  (namespace)

    # Logical
    AMPAMP = auto()        # &&
    PIPEPIPE = auto()      # ||
    QMARK = auto()         # ?
    QQ = auto()            # ??  (nullish coalescing)
    QDOT = auto()          # ?.  (optional chaining)

    # Delimiters
    LPAREN = auto()
    RPAREN = auto()
    LBRACKET = auto()
    RBRACKET = auto()
    LBRACE = auto()
    RBRACE = auto()
    COMMA = auto()
    COLON = auto()
    SEMI = auto()
    AT = auto()            # @ decorator

    # Structural
    NEWLINE = auto()
    EOF = auto()


# Keywords are matched case-insensitively because classic BASIC is upper-case
# oriented but modern users type lower-case. We keep a canonical lower form.
KEYWORDS = {
    "print": "PRINT", "input": "INPUT", "let": "LET", "dim": "DIM",
    "if": "IF", "then": "THEN", "else": "ELSE", "elseif": "ELSEIF",
    "elsif": "ELSEIF", "end": "END", "endif": "END_IF",
    "for": "FOR", "to": "TO", "step": "STEP", "next": "NEXT",
    "while": "WHILE", "wend": "WEND", "do": "DO", "loop": "LOOP",
    "repeat": "REPEAT", "until": "UNTIL",
    "goto": "GOTO", "gosub": "GOSUB", "return": "RETURN",
    "rem": "REM",
    "select": "SELECT", "case": "CASE",
    "function": "FUNCTION", "fn": "FN", "sub": "SUB", "def": "DEF",
    "lambda": "LAMBDA",
    "class": "CLASS", "extends": "EXTENDS", "new": "NEW", "me": "ME",
    "self": "ME",
    "match": "MATCH", "when": "WHEN", "is": "IS", "in": "IN",
    "async": "ASYNC", "await": "AWAIT", "spawn": "SPAWN", "go": "GO",
    "try": "TRY", "catch": "CATCH", "finally": "FINALLY",
    "throw": "THROW", "raise": "THROW",
    "import": "IMPORT", "from": "FROM", "export": "EXPORT", "as": "AS",
    "const": "CONST", "var": "VAR",
    "type": "TYPE", "enum": "ENUM", "trait": "TRAIT", "impl": "IMPL",
    "struct": "STRUCT",
    "and": "AND", "or": "OR", "not": "NOT", "mod": "MOD", "div": "DIV",
    "xor": "XOR", "shl": "SHL", "shr": "SHR",
    "true": "TRUE", "false": "FALSE", "none": "NONE", "nil": "NONE",
    "null": "NONE",
    "break": "BREAK", "continue": "CONTINUE", "pass": "PASS", "stop": "STOP",
    "yield": "YIELD", "defer": "DEFER", "chan": "CHAN",
    "with": "WITH",
    "exit": "EXIT",
    "option": "OPTION", "base": "BASE",
    "data": "DATA", "read": "READ", "restore": "RESTORE",
    "randomize": "RANDOMIZE", "using": "USING",
    "on": "ON", "error": "ERROR", "resume": "RESUME", "shared": "SHARED",
    "local": "LOCAL", "global": "GLOBAL",
    "public": "PUBLIC", "private": "PRIVATE", "static": "STATIC",
    "abstract": "ABSTRACT", "override": "OVERRIDE", "virtual": "VIRTUAL",
    "constructor": "CONSTRUCTOR", "ctor": "CONSTRUCTOR", "init": "CONSTRUCTOR",
}


class Token:
    __slots__ = ("type", "value", "line", "col", "kw")

    def __init__(self, type: TokenType, value, line: int, col: int, kw: str = ""):
        self.type = type
        self.value = value
        self.line = line
        self.col = col
        self.kw = kw

    def __repr__(self):
        return f"Token({self.type.name}, {self.value!r}, line={self.line})"

    def __eq__(self, other):
        if isinstance(other, TokenType):
            return self.type == other
        return NotImplemented
