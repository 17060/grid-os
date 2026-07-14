"""The GridBasic lexer.

Turns source text into a flat list of :class:`Token` objects. Highlights:

* Classic BASIC line numbers (``10 PRINT "hi"``) become ``LABEL`` markers so
  ``GOTO 10`` keeps working.
* Modern ``f"hello {name}"`` interpolated strings are split into segments so
  the parser can embed real expressions.
* ``re"pattern"`` raw regex literals.
* Line continuation with a trailing ``_`` or ``\\`` (classic BASIC style).
* ``REM`` and ``'`` comments.
"""

from .tokens import Token, TokenType, KEYWORDS
from .errors import LexError


_ESCAPES = {
    "n": "\n", "t": "\t", "r": "\r", "0": "\0",
    "\\": "\\", '"': '"', "'": "'",
    "a": "\a", "b": "\b", "f": "\f", "v": "\v",
}


class Lexer:
    def __init__(self, source: str):
        self.src = source
        self.pos = 0
        self.line = 1
        self.col = 1
        self.tokens: list[Token] = []
        # True when the previous emitted token allows a leading line-number
        # label (start of a statement / start of file / right after NEWLINE).
        self._at_line_start = True

    # ---- low-level helpers ------------------------------------------------
    def _peek(self, off=0):
        i = self.pos + off
        return self.src[i] if i < len(self.src) else ""

    def _advance(self):
        c = self.src[self.pos]
        self.pos += 1
        if c == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return c

    def _eof(self):
        return self.pos >= len(self.src)

    def _emit(self, type: TokenType, value, kw: str = ""):
        self.tokens.append(Token(type, value, self.line, self.col, kw))

    # ---- main loop --------------------------------------------------------
    def tokenize(self) -> list[Token]:
        depth = 0  # paren/bracket/brace nesting — newlines inside are ignored
        while not self._eof():
            c = self._peek()
            if c in " \t\r":
                self._advance()
                continue
            # Line continuation: trailing _ or \ before newline
            if c in "_\\" and self._peek(1) == "\n":
                self._advance(); self._advance()
                continue
            if c == "\n":
                if depth == 0:
                    if self.tokens and self.tokens[-1].type != TokenType.NEWLINE:
                        self._emit(TokenType.NEWLINE, "\\n")
                    self._at_line_start = True
                self._advance()
                continue
            # Comments: REM ... or ' ...
            if c == "'":
                self._skip_to_eol()
                continue
            if self._is_word(c) and self._peek_word() == "rem":
                self._skip_to_eol()
                continue
            # Classic line-number label at line start: 10 PRINT ...
            if c.isdigit() and self._at_line_start:
                if self._consume_line_label():
                    continue
            # Numbers
            if c.isdigit() or (c == "." and self._peek(1).isdigit()):
                self._lex_number()
                self._at_line_start = False
                continue
            # Strings (incl f-strings and regex)
            if c == '"':
                self._lex_string(double=True)
                self._at_line_start = False
                continue
            if c == "'":
                # single-quoted string handled below — but above we used ' for
                # comments. To allow single-quote strings we'd need context.
                # We treat ' as comment (classic BASIC). Use " for strings.
                self._skip_to_eol()
                continue
            # f-strings: f"..." or $"..."
            if (c in "fF$") and self._peek(1) == '"':
                self._advance()  # consume f / F / $
                if self._peek() == "$":  # $"..." form
                    self._advance()
                self._lex_string(double=True, interpolated=True)
                self._at_line_start = False
                continue
            # regex literal: re"..." or r"..."
            if (c in "rR") and self._peek(1) == '"':
                # only treat as regex if preceded by an operator / start, to
                # avoid swallowing identifiers beginning with 'r'.
                ok = (not self.tokens) or self.tokens[-1].type in (
                    TokenType.NEWLINE, TokenType.LPAREN, TokenType.LBRACKET,
                    TokenType.LBRACE, TokenType.COMMA, TokenType.SEMI,
                    TokenType.EQ, TokenType.EQEQ, TokenType.NEQ, TokenType.LT,
                    TokenType.GT, TokenType.LE, TokenType.GE, TokenType.PLUS,
                    TokenType.MINUS, TokenType.STAR, TokenType.SLASH,
                    TokenType.PERCENT, TokenType.AMPAMP, TokenType.PIPEPIPE,
                    TokenType.COLON, TokenType.ARROW, TokenType.SEND,
                ) or self.tokens[-1].type == TokenType.KEYWORD
                if ok:
                    self._advance()  # consume r
                    self._lex_string(double=True, regex=True)
                    self._at_line_start = False
                    continue
            # Identifiers / keywords
            if self._is_word(c):
                self._lex_word()
                self._at_line_start = False
                continue
            # Operators / punctuation
            self._lex_op()
            self._at_line_start = False
            # track bracket depth for newline suppression
            lt = self.tokens[-1].type
            if lt in (TokenType.LPAREN, TokenType.LBRACKET, TokenType.LBRACE):
                depth += 1
            elif lt in (TokenType.RPAREN, TokenType.RBRACKET, TokenType.RBRACE):
                if depth > 0:
                    depth -= 1

        # Final newline so the parser always sees a terminator
        if self.tokens and self.tokens[-1].type != TokenType.NEWLINE:
            self._emit(TokenType.NEWLINE, "\\n")
        self._emit(TokenType.EOF, None)
        return self.tokens

    # ---- helpers ----------------------------------------------------------
    def _is_word(self, c):
        return c.isalnum() or c == "_" or c >= chr(128)

    def _peek_word(self):
        i = self.pos
        out = []
        while i < len(self.src) and (self.src[i].isalnum() or self.src[i] == "_"):
            out.append(self.src[i]); i += 1
        return "".join(out).lower()

    def _skip_to_eol(self):
        while not self._eof() and self._peek() != "\n":
            self._advance()

    def _consume_line_label(self):
        start = self.pos
        while not self._eof() and self._peek().isdigit():
            self._advance()
        num = self.src[start:self.pos]
        # Must be followed by whitespace (or newline) to be a line label.
        nxt = self._peek()
        if nxt in " \t\r\n" or nxt == "":
            self.tokens.append(Token(TokenType.INT, int(num), self.line, self.col))
            # Mark it as a label by immediately emitting a synthetic LABEL via
            # a special keyword token "LABEL" — we use the keyword channel so
            # the parser can recognize it. Cheaper: parser sees INT at stmt
            # start and treats it as a label.
            # We emit a colon-equivalent by setting kw.
            self.tokens[-1].kw = "LABEL"
            return True
        # Otherwise it's a normal number — rewind handled by falling through.
        self.pos = start
        return False

    def _lex_number(self):
        start = self.pos
        nx = self._peek(1)
        if self._peek() == "0" and nx in ("x", "X"):
            self._advance(); self._advance()
            dig = ""
            while not self._eof() and self._peek() in "0123456789abcdefABCDEF":
                dig += self._advance()
            self._emit(TokenType.INT, int(dig or "0", 16))
            return
        if self._peek() == "0" and nx in ("b", "B"):
            self._advance(); self._advance()
            dig = ""
            while not self._eof() and self._peek() in "01":
                dig += self._advance()
            self._emit(TokenType.INT, int(dig or "0", 2))
            return
        is_float = False
        while not self._eof() and self._peek().isdigit():
            self._advance()
        nx = self._peek(1)
        if self._peek() == "." and nx.isdigit():
            is_float = True
            self._advance()
            while not self._eof() and self._peek().isdigit():
                self._advance()
        cur = self._peek()
        if cur in ("e", "E") and not self._eof():
            is_float = True
            self._advance()
            if self._peek() in ("+", "-"):
                self._advance()
            while not self._eof() and self._peek().isdigit():
                self._advance()
        text = self.src[start:self.pos]
        if is_float:
            self._emit(TokenType.FLOAT, float(text))
        else:
            self._emit(TokenType.INT, int(text))

    def _lex_string(self, double=True, interpolated=False, regex=False):
        quote = '"'
        # opening quote
        self._advance()
        segs = []
        buf = []
        if not interpolated:
            # plain string (or regex) — read with escapes unless regex
            raw = regex
            while not self._eof():
                c = self._peek()
                if c == quote:
                    if raw:
                        # check for "" escape in regex? raw means no escapes
                        self._advance()
                        text = "".join(buf)
                        if regex:
                            self._emit(TokenType.REGEX, text)
                        else:
                            self._emit(TokenType.STRING, text)
                        return
                    self._advance()
                    text = "".join(buf)
                    self._emit(TokenType.STRING, text)
                    return
                if c == "\\" and not raw:
                    self._advance()
                    e = self._advance()
                    buf.append(_ESCAPES.get(e, e))
                    continue
                if c == "\n":
                    raise LexError("Unterminated string literal", self.line, self.col)
                buf.append(self._advance())
            raise LexError("Unterminated string literal", self.line, self.col)
        else:
            # interpolated f-string: parse {expr} segments, {{ => literal {
            while not self._eof():
                c = self._peek()
                if c == quote:
                    self._advance()
                    if buf:
                        segs.append(("lit", "".join(buf)))
                    self._emit(TokenType.FSTRING, segs)
                    return
                if c == "{" and self._peek(1) == "{":
                    buf.append("{"); self._advance(); self._advance(); continue
                if c == "}" and self._peek(1) == "}":
                    buf.append("}"); self._advance(); self._advance(); continue
                if c == "{":
                    if buf:
                        segs.append(("lit", "".join(buf))); buf = []
                    self._advance()
                    expr_src, fmt = self._read_interp()
                    segs.append(("expr", expr_src, fmt))
                    continue
                if c == "\\":
                    self._advance()
                    e = self._advance()
                    buf.append(_ESCAPES.get(e, e))
                    continue
                if c == "\n":
                    raise LexError("Unterminated f-string", self.line, self.col)
                buf.append(self._advance())
            raise LexError("Unterminated f-string", self.line, self.col)

    def _read_interp(self):
        """Read until the closing } of an interpolation. Supports a format
        suffix ``{expr:fmt}`` (the fmt is captured as a string)."""
        depth = 1
        start = self.pos
        fmt = None
        while not self._eof():
            c = self._peek()
            if c == "{":
                depth += 1; self._advance(); continue
            if c == "}":
                depth -= 1
                if depth == 0:
                    src = self.src[start:self.pos]
                    self._advance()
                    return src, fmt
                self._advance(); continue
            if c == ":" and depth == 1 and fmt is None:
                src = self.src[start:self.pos]
                self._advance()
                # read fmt until matching }
                fstart = self.pos
                while not self._eof() and self._peek() != "}":
                    self._advance()
                fmt = self.src[fstart:self.pos]
                # closing } consumed by outer loop on next iteration
                # but we need to consume it here
                if self._peek() == "}":
                    self._advance()
                return src, fmt
            if c == "\n":
                raise LexError("Unterminated interpolation", self.line, self.col)
            self._advance()
        raise LexError("Unterminated interpolation", self.line, self.col)

    def _lex_word(self):
        start = self.pos
        while not self._eof() and self._is_word(self._peek()):
            self._advance()
        # allow trailing ? and $ and % sigils (classic BASIC type sigils)
        if not self._eof() and self._peek() in ("$", "%"):
            self._advance()
        word = self.src[start:self.pos]
        low = word.lower().rstrip("$%")
        if low in KEYWORDS and word.rstrip("$%").lower() == low:
            # type-sigil on a keyword? only allow for non-sigil keywords
            kw = KEYWORDS[low]
            self.tokens.append(Token(TokenType.KEYWORD, kw, self.line, self.col, kw))
        else:
            self._emit(TokenType.IDENT, word)

    def _lex_op(self):
        c = self._peek()
        nxt = self._peek(1)
        # multi-char first
        two = c + nxt
        three = two + self._peek(2)
        if three == "...":
            self._advance(); self._advance(); self._advance()
            self._emit(TokenType.DOTDOTDOT, "..."); return
        if two == "..":
            self._advance(); self._advance()
            if self._peek() == "=":
                self._advance(); self._emit(TokenType.DOTDOTEQ, "..=")
            else:
                self._emit(TokenType.DOTDOT, "..")
            return
        if two == "==" :
            self._advance(); self._advance(); self._emit(TokenType.EQEQ, "=="); return
        if two == "!=" or two == "<>":
            self._advance(); self._advance(); self._emit(TokenType.NEQ, two); return
        if two == "<=":
            self._advance(); self._advance(); self._emit(TokenType.LE, "<="); return
        if two == ">=":
            self._advance(); self._advance(); self._emit(TokenType.GE, ">="); return
        if two == "&&":
            self._advance(); self._advance(); self._emit(TokenType.AMPAMP, "&&"); return
        if two == "||":
            self._advance(); self._advance(); self._emit(TokenType.PIPEPIPE, "||"); return
        if two == "??":
            self._advance(); self._advance(); self._emit(TokenType.QQ, "??"); return
        if two == "?.":
            self._advance(); self._advance(); self._emit(TokenType.QDOT, "?."); return
        if two == ":=":
            self._advance(); self._advance(); self._emit(TokenType.COLONEQ, ":="); return
        if two == "=>":
            self._advance(); self._advance(); self._emit(TokenType.ARROW, "=>"); return
        if two == "<-":
            self._advance(); self._advance(); self._emit(TokenType.SEND, "<-"); return
        if two == "::":
            self._advance(); self._advance(); self._emit(TokenType.COLONCOLON, "::"); return
        # single
        singles = {
            "+": TokenType.PLUS, "-": TokenType.MINUS, "*": TokenType.STAR,
            "/": TokenType.SLASH, "%": TokenType.PERCENT, "^": TokenType.CARET,
            ".": TokenType.DOT, "=": TokenType.EQ, "<": TokenType.LT,
            ">": TokenType.GT, "(": TokenType.LPAREN, ")": TokenType.RPAREN,
            "[": TokenType.LBRACKET, "]": TokenType.RBRACKET,
            "{": TokenType.LBRACE, "}": TokenType.RBRACE, ",": TokenType.COMMA,
            ":": TokenType.COLON, ";": TokenType.SEMI, "@": TokenType.AT,
            "?": TokenType.QMARK,
        }
        if c in singles:
            self._advance()
            self._emit(singles[c], c); return
        raise LexError(f"Unexpected character {c!r}", self.line, self.col)


def _is_ident_char(c):
    return c.isalnum() or c == "_"
