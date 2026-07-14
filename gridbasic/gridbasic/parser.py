"""The GridBasic parser.

A hand-written recursive-descent parser with a Pratt-style precedence climber
for expressions. It produces a :class:`ast_nodes.Program` ready for the
tree-walking interpreter.

The grammar is line-oriented (statements separated by NEWLINE or ``:``) but
also supports modern block forms (``IF ... END IF``, ``FUNCTION ... END
FUNCTION`` etc.). Single-line ``IF ... THEN stmt`` keeps the classic BASIC
feel.
"""

from __future__ import annotations

from .tokens import Token, TokenType
from . import ast_nodes as ast
from .errors import ParseError


COMPOUND_OPS = {"+": "+", "-": "-", "*": "*", "/": "/", "%": "%", "^": "^"}
ASSIGN_TOKENS = {TokenType.EQ, TokenType.COLONEQ}


class Parser:
    def __init__(self, tokens: list[Token]):
        self.toks = tokens
        self.i = 0
        self.labels: dict[str, int] = {}

    # ---- token helpers ----------------------------------------------------
    def _peek(self, off=0) -> Token:
        j = self.i + off
        if j >= len(self.toks):
            return self.toks[-1]
        return self.toks[j]

    def _cur(self) -> Token:
        return self.toks[self.i]

    def _at(self, ttype) -> bool:
        return self._cur().type == ttype

    def _at_kw(self, *names) -> bool:
        t = self._cur()
        return t.type == TokenType.KEYWORD and t.kw in names

    def _advance(self) -> Token:
        t = self.toks[self.i]
        if t.type != TokenType.EOF:
            self.i += 1
        return t

    def _expect(self, ttype, what=""):
        if self._cur().type != ttype:
            raise ParseError(
                f"Expected {what or ttype.name}, got {self._cur().type.name} "
                f"({self._cur().value!r})", self._cur().line, self._cur().col)
        return self._advance()

    def _expect_kw(self, name, what=""):
        if not self._at_kw(name):
            raise ParseError(
                f"Expected {what or name}, got {self._cur().value!r}",
                self._cur().line, self._cur().col)
        return self._advance()

    def _skip_newlines(self):
        while self._at(TokenType.NEWLINE) or self._at_kw("PASS"):
            if self._at_kw("PASS"):
                self._advance(); continue
            self._advance()

    def _skip_sep(self):
        """Skip statement separators: NEWLINE, ':' or ';' (BASIC allows ;)."""
        while self._at(TokenType.NEWLINE) or self._at(TokenType.COLON) or self._at(TokenType.SEMI):
            self._advance()

    # ---- entry ------------------------------------------------------------
    def parse(self) -> ast.Program:
        body = []
        self._skip_newlines()
        while not self._at(TokenType.EOF):
            stmt = self.parse_statement()
            if stmt is not None:
                if stmt.kind == "label":
                    self.labels[stmt.name] = len(body)
                else:
                    body.append(stmt)
            self._skip_sep()
            self._skip_newlines()
        return ast.Program(body, self.labels)

    # ---- statements -------------------------------------------------------
    def parse_statement(self):
        t = self._cur()
        # Labels (classic line numbers)
        if t.type == TokenType.INT and t.kw == "LABEL":
            self._advance()
            return ast.LabelDecl(str(t.value), t.line)
        if t.type == TokenType.KEYWORD:
            if t.kw == "ME":
                return self.parse_me_statement()
            return self.parse_keyword_statement()
        # assignment forms
        if t.type == TokenType.IDENT:
            return self.parse_ident_statement()
        if t.type == TokenType.LBRACKET or t.type == TokenType.LBRACE:
            # destructuring assignment: [a,b] = expr  / {x,y} = expr
            return self.parse_destructure_statement()
        if t.type == TokenType.AT:
            # decorated definition
            return self.parse_decorated()
        # bare expression statement
        expr = self.parse_expression()
        return ast.ExprStmt(expr, t.line)

    def parse_decorated(self):
        decorators = []
        while self._at(TokenType.AT):
            self._advance()
            name = self._expect(TokenType.IDENT, "decorator name").value
            args, kwargs = [], {}
            if self._at(TokenType.LPAREN):
                args, kwargs = self.parse_call_args()
            decorators.append((name, args, kwargs))
            self._skip_newlines()
        if self._at_kw("ASYNC", "FUNCTION", "SUB", "DEF"):
            return self.parse_function(decorators=decorators)
        if self._at_kw("CLASS"):
            return self.parse_class(decorators=decorators)
        if self._at_kw("FN"):
            return self.parse_fn_arrow_binding(decorators=decorators)
        raise ParseError("Decorator must precede FUNCTION/SUB/CLASS",
                         self._cur().line, self._cur().col)

    def parse_me_statement(self):
        """Handle ME.field = value (and ME[i] = v, ME.a.b = v) assignments,
        since ME is a keyword and wouldn't otherwise route through the
        assignment path."""
        line = self._cur().line
        target = self.parse_postfix()
        t = self._cur()
        if t.type == TokenType.EQ:
            self._advance()
            value = self.parse_expression()
            return ast.Assign(target, value, line)
        if (t.type in (TokenType.PLUS, TokenType.MINUS, TokenType.STAR,
                       TokenType.SLASH, TokenType.PERCENT, TokenType.CARET)
                and self._peek(1).type == TokenType.EQ):
            op = t.value
            self._advance(); self._advance()
            value = self.parse_expression()
            return ast.AssignOp(op, target, value, line)
        if t.type == TokenType.COLONEQ:
            self._advance()
            value = self.parse_expression()
            return ast.Assign(target, value, line)
        return ast.ExprStmt(target, line)

    def parse_keyword_statement(self):
        kw = self._cur().kw
        # 'fn' is the arrow-binding form (fn name = expr / fn name(a,b) => expr).
        if kw == "FN":
            return self.parse_fn_arrow_binding()
        # Definition keywords own their leading keyword (and optional ASYNC),
        # so we do NOT pre-advance here — the handler consumes it itself.
        if kw in ("ASYNC", "FUNCTION", "SUB", "DEF", "CLASS", "ENUM"):
            return self._kw_handlers[kw](self)
        handler = self._kw_handlers.get(kw)
        if handler is None:
            # Expression-leading keywords used in statement position.
            if kw in ("MATCH", "AWAIT", "NEW", "LAMBDA", "SPAWN", "GO",
                      "TRUE", "FALSE", "NONE", "ME", "CHAN", "NOT", "THROW",
                      "YIELD", "DEFER"):
                return self.parse_expression_statement_kw(kw)
            raise ParseError(f"Unexpected keyword {kw}", self._cur().line, self._cur().col)
        self._advance()  # consume the keyword for simple handlers
        return handler(self)

    def parse_expression_statement_kw(self, kw):
        if kw == "THROW":
            self._advance()
            val = self.parse_expression()
            return ast.ThrowStmt(val, self._cur().line)
        if kw == "YIELD":
            self._advance()
            val = None
            if not (self._at(TokenType.NEWLINE) or self._at(TokenType.COLON)
                    or self._at(TokenType.SEMI) or self._at(TokenType.EOF)):
                val = self.parse_expression()
            return ast.YieldStmt(val, self._cur().line)
        if kw == "DEFER":
            self._advance()
            expr = self.parse_expression()
            return ast.DeferStmt(expr, self._cur().line)
        # others fall through to expression statement
        expr = self.parse_expression()
        return ast.ExprStmt(expr, self._cur().line)

    def parse_fn_arrow_binding(self, decorators=None):
        decs = decorators or []
        self._expect_kw("FN")
        name = self._expect(TokenType.IDENT, "function name").value
        # Form 1: fn name = <expr>   (expr is often a lambda)
        if self._at(TokenType.EQ):
            self._advance()
            body = self.parse_expression()
            if isinstance(body, ast.Lambda):
                if body.expr_form:
                    body_nodes = [ast.ReturnStmt(body.body)]
                else:
                    body_nodes = body.body
                return ast.FuncDef(name, body.params, body_nodes, is_async=False,
                                   line=self._cur().line, decorators=decs)
            return ast.FuncDef(name, [], [ast.ReturnStmt(body)], is_async=False,
                               line=self._cur().line, decorators=decs)
        # Form 2: fn name(params) => expr   /  fn name a, b => expr
        params = []
        if self._at(TokenType.LPAREN):
            params = self.parse_param_list()
        elif self._at(TokenType.IDENT):
            params.append(self._parse_one_param())
            while self._at(TokenType.COMMA):
                self._advance()
                params.append(self._parse_one_param())
        self._expect(TokenType.ARROW, "'=>'")
        body = self.parse_expression()
        return ast.FuncDef(name, params, [ast.ReturnStmt(body)], is_async=False,
                           line=self._cur().line, decorators=decs)

    # ---- assignment / declaration statements ------------------------------
    def parse_ident_statement(self):
        # Could be: assignment, compound assignment, call statement, method call, indexed assignment
        # We parse a postfix expression and check for '=' or compound op.
        start_i = self.i
        line = self._cur().line
        # Peek ahead to decide: is this an assignment? Find next = or compound-assign at top level.
        if self._is_assignment_ahead():
            target = self.parse_postfix()
            t = self._cur()
            if t.type == TokenType.EQ:
                self._advance()
                value = self.parse_expression()
                return ast.Assign(target, value, line)
            # compound op: + - * / % ^ followed by =
            if (t.type in (TokenType.PLUS, TokenType.MINUS, TokenType.STAR,
                           TokenType.SLASH, TokenType.PERCENT, TokenType.CARET)
                    and self._peek(1).type == TokenType.EQ):
                op = t.value
                self._advance(); self._advance()
                value = self.parse_expression()
                return ast.AssignOp(op, target, value, line)
            if t.type == TokenType.COLONEQ:
                self._advance()
                value = self.parse_expression()
                return ast.Assign(target, value, line)
            raise ParseError("Invalid assignment target", t.line, t.col)
        # Not an assignment — expression statement (call, etc.)
        expr = self.parse_expression()
        return ast.ExprStmt(expr, line)

    def _is_assignment_ahead(self):
        """Look ahead from current position to see if this is an assignment.
        We scan postfix (idents, dots, brackets, parens) and check whether the
        next top-level token is '=' or a compound assign."""
        depth = 0
        j = self.i
        n = len(self.toks)
        while j < n:
            t = self.toks[j]
            if t.type == TokenType.EOF:
                return False
            if depth == 0:
                if t.type in ASSIGN_TOKENS:
                    return True
                if (t.type in (TokenType.PLUS, TokenType.MINUS, TokenType.STAR,
                               TokenType.SLASH, TokenType.PERCENT, TokenType.CARET)
                        and j + 1 < n and self.toks[j+1].type == TokenType.EQ):
                    return True
                if t.type in (TokenType.NEWLINE, TokenType.COLON, TokenType.SEMI):
                    return False
                # Stop if we hit a binary operator that's NOT compound-assign
                # at depth 0 — that means this is an expression, not assignment.
                if t.type in (TokenType.EQEQ, TokenType.NEQ, TokenType.LT, TokenType.GT,
                              TokenType.LE, TokenType.GE, TokenType.AMPAMP, TokenType.PIPEPIPE,
                              TokenType.QQ, TokenType.ARROW, TokenType.DOTDOT,
                              TokenType.DOTDOTEQ, TokenType.SEND, TokenType.QMARK):
                    return False
                if t.type == TokenType.KEYWORD and t.kw in (
                        "AND", "OR", "MOD", "DIV", "XOR", "SHL", "SHR", "IN",
                        "IS", "TO", "STEP", "THEN", "ELSE", "ELSEIF", "FOR",
                        "WHILE", "DO", "IF", "FUNCTION", "SUB", "CLASS", "RETURN"):
                    return False
            if t.type in (TokenType.LPAREN, TokenType.LBRACKET, TokenType.LBRACE):
                depth += 1
            elif t.type in (TokenType.RPAREN, TokenType.RBRACKET, TokenType.RBRACE):
                depth -= 1
            elif t.type == TokenType.KEYWORD and depth == 0:
                return False
            j += 1
        return False

    def parse_destructure_statement(self):
        line = self._cur().line
        if self._at(TokenType.LBRACKET):
            self._advance()
            names = []
            while not self._at(TokenType.RBRACKET):
                if self._at(TokenType.DOTDOTDOT):
                    self._advance()
                    nm = self._expect(TokenType.IDENT).value
                    names.append(("rest", nm))
                else:
                    nm = self._expect(TokenType.IDENT).value
                    names.append(("name", nm))
                if self._at(TokenType.COMMA): self._advance()
            self._expect(TokenType.RBRACKET, "']'")
            self._expect(TokenType.EQ, "'='")
            value = self.parse_expression()
            return ast.DestructureStmt(names, value, "list", line)
        # brace destructure
        self._expect(TokenType.LBRACE, "'{'")
        names = []
        while not self._at(TokenType.RBRACE):
            key = self._expect(TokenType.IDENT).value
            if self._at(TokenType.COLON):
                self._advance()
                nm = self._expect(TokenType.IDENT).value
            else:
                nm = key
            names.append(("key", key, nm))
            if self._at(TokenType.COMMA): self._advance()
        self._expect(TokenType.RBRACE, "'}'")
        self._expect(TokenType.EQ, "'='")
        value = self.parse_expression()
        return ast.DestructureStmt(names, value, "dict", line)

    # ---- keyword statement handlers ---------------------------------------
    def parse_print(self):
        line = self._cur().line
        parts = []  # list of (expr, sep)
        using = None
        if self._at_kw("USING"):
            self._advance()
            using = self.parse_expression()
            self._expect(TokenType.SEMI, "';'")
        newline = True
        if (self._at(TokenType.NEWLINE) or self._at(TokenType.COLON)
                or self._at(TokenType.SEMI) or self._at(TokenType.EOF)
                or self._at(TokenType.EOF)):
            return ast.PrintStmt([], newline, using, line)
        while True:
            expr = self.parse_expression()
            sep = ""
            if self._at(TokenType.SEMI):
                sep = ";"; self._advance()
            elif self._at(TokenType.COMMA):
                sep = ","; self._advance()
            parts.append((expr, sep))
            if sep == "":
                break
            if (self._at(TokenType.NEWLINE) or self._at(TokenType.COLON)
                    or self._at(TokenType.EOF)):
                newline = False
                break
        # trailing separator suppresses newline
        if parts and parts[-1][1] in (";", ","):
            newline = False
        return ast.PrintStmt(parts, newline, using, line)

    def parse_input(self):
        line = self._cur().line
        prompt = None
        if self._at(TokenType.STRING):
            prompt = ast.StrLit(self._advance().value)
            self._expect(TokenType.SEMI, "';'")
        elif self._at(TokenType.FSTRING):
            prompt = self.parse_fstring()
            self._expect(TokenType.SEMI, "';'")
        var = self.parse_postfix()
        return ast.InputStmt(prompt, var, line)

    def parse_let(self):
        line = self._cur().line
        is_const = self._cur().kw == "CONST"
        # Destructuring let: let [a,b] = ... or let {x,y} = ...
        if self._at(TokenType.LBRACKET) or self._at(TokenType.LBRACE):
            if self._at(TokenType.LBRACKET):
                self._advance()
                names = []
                while not self._at(TokenType.RBRACKET):
                    if self._at(TokenType.DOTDOTDOT):
                        self._advance()
                        names.append(("rest", self._expect(TokenType.IDENT).value))
                    else:
                        names.append(("name", self._expect(TokenType.IDENT).value))
                    if self._at(TokenType.COMMA): self._advance()
                self._expect(TokenType.RBRACKET, "']'")
                self._expect(TokenType.EQ, "'='")
                value = self.parse_expression()
                return ast.DestructureStmt(names, value, "list", line)
            self._advance()  # {
            names = []
            while not self._at(TokenType.RBRACE):
                key = self._expect(TokenType.IDENT).value
                nm = key
                if self._at(TokenType.COLON):
                    self._advance(); nm = self._expect(TokenType.IDENT).value
                names.append(("key", key, nm))
                if self._at(TokenType.COMMA): self._advance()
            self._expect(TokenType.RBRACE, "'}'")
            self._expect(TokenType.EQ, "'='")
            value = self.parse_expression()
            return ast.DestructureStmt(names, value, "dict", line)
        name = self._expect(TokenType.IDENT, "variable name").value
        type_ann = None
        if self._at(TokenType.COLON):
            self._advance()
            type_ann = self.parse_type()
        elif self._at_kw("AS"):
            self._advance()
            type_ann = self.parse_type()
        if self._at(TokenType.EQ):
            self._advance()
            value = self.parse_expression()
        else:
            value = None
        return ast.LetStmt(name, type_ann, value, is_const, line)

    def parse_dim(self):
        line = self._cur().line
        name = self._expect(TokenType.IDENT, "array name").value
        dims = []
        if self._at(TokenType.LPAREN):
            self._advance()
            while not self._at(TokenType.RPAREN):
                dims.append(self.parse_expression())
                if self._at(TokenType.COMMA): self._advance()
            self._expect(TokenType.RPAREN, "')'")
        elif self._at(TokenType.LBRACKET):
            self._advance()
            while not self._at(TokenType.RBRACKET):
                dims.append(self.parse_expression())
                if self._at(TokenType.COMMA): self._advance()
            self._expect(TokenType.RBRACKET, "']'")
        return ast.DimStmt(name, dims, line)

    def parse_if(self):
        line = self._cur().line
        cond = self.parse_expression()
        self._expect_kw("THEN", "THEN")
        branches = []
        else_body = None
        # single-line IF: THEN followed by statements on same line (no NEWLINE first)
        if not self._at(TokenType.NEWLINE):
            # parse statements until ELSE or end-of-line
            then_body = []
            while not (self._at(TokenType.NEWLINE) or self._at(TokenType.EOF)
                       or self._at_kw("ELSE")):
                stmt = self.parse_statement()
                if stmt is not None:
                    if stmt.kind == "label":
                        self.labels[stmt.name] = len(then_body)  # local; not perfect
                    then_body.append(stmt)
                if self._at(TokenType.COLON) or self._at(TokenType.SEMI):
                    self._advance(); continue
                break
            branches.append((cond, then_body))
            if self._at_kw("ELSE"):
                self._advance()
                else_body = []
                while not (self._at(TokenType.NEWLINE) or self._at(TokenType.EOF)):
                    stmt = self.parse_statement()
                    if stmt is not None:
                        else_body.append(stmt)
                    if self._at(TokenType.COLON) or self._at(TokenType.SEMI):
                        self._advance(); continue
                    break
            return ast.IfStmt(branches, else_body, line)
        # block form
        body = self.parse_block({("ELSE",), ("ELSEIF",), ("END", "IF"), ("ENDIF",)})
        branches.append((cond, body))
        while self._at_kw("ELSEIF"):
            self._advance()
            c = self.parse_expression()
            self._expect_kw("THEN", "THEN")
            b = self.parse_block({("ELSE",), ("ELSEIF",), ("END", "IF"), ("ENDIF",)})
            branches.append((c, b))
        if self._at_kw("ELSE"):
            self._advance()
            else_body = self.parse_block({("END", "IF"), ("ENDIF",)})
        self._expect_terminator(("END", "IF"), ("ENDIF",))
        return ast.IfStmt(branches, else_body, line)

    def parse_for(self):
        line = self._cur().line
        var = self._expect(TokenType.IDENT, "loop variable").value
        # FOR i IN iter  /  FOR i = a TO b [STEP s]
        if self._at_kw("IN"):
            self._advance()
            it = self.parse_expression()
            body = self.parse_block({("NEXT",), ("END", "FOR")})
            self._expect_terminator(("NEXT",), ("END", "FOR"))
            # optional NEXT i
            if self._at(TokenType.IDENT):
                self._advance()
            return ast.ForInStmt(var, it, body, line)
        self._expect(TokenType.EQ, "'='")
        start = self.parse_expression()
        self._expect_kw("TO", "TO")
        end = self.parse_expression()
        step = None
        if self._at_kw("STEP"):
            self._advance()
            step = self.parse_expression()
        body = self.parse_block({("NEXT",), ("END", "FOR")})
        self._expect_terminator(("NEXT",), ("END", "FOR"))
        if self._at(TokenType.IDENT):
            self._advance()
        return ast.ForStmt(var, start, end, step, body, line)

    def parse_while(self):
        line = self._cur().line
        cond = self.parse_expression()
        body = self.parse_block({("WEND",), ("END", "WHILE")})
        self._expect_terminator(("WEND",), ("END", "WHILE"))
        return ast.WhileStmt(cond, body, line)

    def parse_do(self):
        line = self._cur().line
        # DO [WHILE|UNTIL cond] ... LOOP [[WHILE|UNTIL] cond]
        pre = None
        if self._at_kw("WHILE"):
            self._advance(); pre = ("while", self.parse_expression())
        elif self._at_kw("UNTIL"):
            self._advance(); pre = ("until", self.parse_expression())
        body = self.parse_block({("LOOP",)})
        self._expect_kw("LOOP", "LOOP")
        post = None
        if self._at_kw("WHILE"):
            self._advance(); post = ("while", self.parse_expression())
        elif self._at_kw("UNTIL"):
            self._advance(); post = ("until", self.parse_expression())
        # We model DO as a while with pre/post; encode via RepeatStmt/WhileStmt.
        if pre is None and post is None:
            # infinite loop — use while True with a break expectation
            return ast.WhileStmt(ast.BoolLit(True, line), body, line)
        if pre is not None and post is None:
            if pre[0] == "while":
                return ast.WhileStmt(pre[1], body, line)
            else:
                return ast.RepeatStmt(body, ast.UnaryOp("not", pre[1], line), line)
        # post-test: repeat-until style
        if post is not None:
            cond = post[1] if post[0] == "until" else ast.UnaryOp("not", post[1], line)
            return ast.RepeatStmt(body, cond, line)
        return ast.WhileStmt(ast.BoolLit(True, line), body, line)

    def parse_repeat(self):
        line = self._cur().line
        body = self.parse_block({("UNTIL",)})
        self._expect_kw("UNTIL", "UNTIL")
        cond = self.parse_expression()
        return ast.RepeatStmt(body, cond, line)

    def parse_select(self):
        line = self._cur().line
        self._expect_kw("CASE", "CASE")
        expr = self.parse_expression()
        cases = []
        else_body = None
        self._skip_newlines()
        while self._at_kw("CASE"):
            self._advance()
            if self._at_kw("ELSE"):
                self._advance()
                else_body = self.parse_block({("END", "SELECT"), ("CASE",)})
                break
            vals = [self.parse_expression()]
            while self._at(TokenType.COMMA):
                self._advance()
                vals.append(self.parse_expression())
            # optional WHEN guard? skip 'IF' guard
            guard = None
            if self._at_kw("IF"):
                self._advance()
                guard = self.parse_expression()
            body = self.parse_block({("CASE",), ("END", "SELECT")})
            cases.append((vals, guard, body))
            self._skip_newlines()
        self._expect_terminator(("END", "SELECT"))
        return ast.SelectStmt(expr, cases, else_body, line)

    def parse_match(self):
        line = self._cur().line
        scrut = self.parse_expression()
        arms = []
        self._skip_newlines()
        while self._at_kw("CASE"):
            self._advance()
            if self._at_kw("ELSE"):
                self._advance()
                self._expect(TokenType.ARROW, "'=>'")
                body = self.parse_block({("END", "MATCH"), ("CASE",)})
                arms.append((ast.WildcardPat(), None, body))
                self._skip_newlines()
                continue
            pats = [self.parse_pattern()]
            while self._at(TokenType.COMMA):
                self._advance()
                pats.append(self.parse_pattern())
            guard = None
            if self._at_kw("IF"):
                self._advance()
                guard = self.parse_expression()
            self._expect(TokenType.ARROW, "'=>'")
            body = self.parse_block({("CASE",), ("END", "MATCH")})
            arms.append((ast.OrPat(pats) if len(pats) > 1 else pats[0], guard, body))
            self._skip_newlines()
        self._expect_terminator(("END", "MATCH"))
        return ast.MatchStmt(scrut, arms, line)

    def parse_function(self, decorators=None, is_async=False):
        line = self._cur().line
        if self._at_kw("ASYNC"):
            self._advance(); is_async = True
        # Consume the definition keyword if we haven't already.
        if self._at_kw("FUNCTION", "FN", "SUB", "DEF"):
            def_kw = self._advance().kw
        else:
            prev = self._cur(-1)
            def_kw = prev.kw if prev.type == TokenType.KEYWORD else "FUNCTION"
        is_sub = def_kw == "SUB"
        name = self._expect(TokenType.IDENT, "function name").value
        params = self.parse_param_list()
        if is_sub:
            body = self.parse_block({("END", "SUB")})
            self._expect_terminator(("END", "SUB"))
            return ast.SubDef(name, params, body, decorators, line)
        body = self.parse_block({("END", "FUNCTION"), ("END", "FN"), ("END", "DEF")})
        self._expect_terminator(("END", "FUNCTION"), ("END", "FN"), ("END", "DEF"))
        # generator if body contains a yield at any depth (simple scan via a flag)
        is_gen = _contains_yield(body)
        return ast.FuncDef(name, params, body, is_async, is_gen, decorators, line)

    def parse_param_list(self):
        self._expect(TokenType.LPAREN, "'('")
        params = []
        if self._at(TokenType.RPAREN):
            self._advance(); return params
        while True:
            params.append(self._parse_one_param())
            if self._at(TokenType.COMMA):
                self._advance(); continue
            break
        self._expect(TokenType.RPAREN, "')'")
        return params

    def _parse_one_param(self):
        kind = "normal"
        if self._at(TokenType.STAR):
            self._advance(); kind = "star"
        elif self._at(TokenType.DOTDOTDOT):
            self._advance(); kind = "star"
        elif self._at_kw("MOD"):  # not used; placeholder
            pass
        name = self._expect(TokenType.IDENT, "parameter name").value
        # **kwargs via STAR STAR? we don't tokenize ** — use a single star star? Use 'kwargs' kw? Use 'STAR' 'STAR'
        type_ann = None
        if self._at(TokenType.COLON):
            self._advance()
            type_ann = self.parse_type()
        elif self._at_kw("AS"):
            self._advance()
            type_ann = self.parse_type()
        default = None
        if self._at(TokenType.EQ):
            self._advance()
            default = self.parse_expression()
        return (name, default, kind, type_ann)

    def parse_class(self, decorators=None):
        line = self._cur().line
        self._advance()  # CLASS
        name = self._expect(TokenType.IDENT, "class name").value
        base = None
        if self._at_kw("EXTENDS"):
            self._advance()
            base = self.parse_postfix()
        members = []
        self._skip_newlines()
        while not self._at_kw("END"):
            if self._at(TokenType.EOF):
                raise ParseError("Unterminated class", line, 0)
            members.append(self.parse_class_member())
            self._skip_newlines()
        self._expect_terminator(("END", "CLASS"))
        return ast.ClassDef(name, base, members, decorators, line)

    def parse_class_member(self):
        # visibility / static modifiers
        is_static = False
        if self._at_kw("STATIC"):
            self._advance(); is_static = True
        if self._at_kw("PUBLIC") or self._at_kw("PRIVATE"):
            self._advance()
        if self._at_kw("VIRTUAL") or self._at_kw("ABSTRACT") or self._at_kw("OVERRIDE"):
            self._advance()
        if self._at_kw("CONSTRUCTOR") or self._at_kw("DEF"):
            # constructor: CONSTRUCTOR(params) ... END CONSTRUCTOR  (or DEF NEW)
            ctor_kw = self._cur().kw
            self._advance()
            params = self.parse_param_list()
            body = self.parse_block({("END", "CONSTRUCTOR"), ("END", "DEF")})
            self._expect_terminator(("END", "CONSTRUCTOR"), ("END", "DEF"))
            return ("ctor", params, body, is_static)
        if self._at_kw("FUNCTION", "FN", "SUB"):
            fn = self.parse_function()
            return ("method", fn, is_static)
        if self._at_kw("CLASS"):
            # nested class? skip for simplicity
            nested = self.parse_class()
            return ("nested", nested)
        # field declaration: name [AS type] [= default]
        field_name = self._expect(TokenType.IDENT, "field name").value
        type_ann = None
        if self._at_kw("AS"):
            self._advance()
            type_ann = self.parse_type()
        default = None
        if self._at(TokenType.EQ):
            self._advance()
            default = self.parse_expression()
        return ("field", field_name, type_ann, default, is_static)

    def parse_enum(self):
        line = self._cur().line
        self._advance()
        name = self._expect(TokenType.IDENT, "enum name").value
        variants = []
        self._skip_newlines()
        while not self._at_kw("END"):
            v = self._expect(TokenType.IDENT, "variant name").value
            val = None
            if self._at(TokenType.EQ):
                self._advance()
                val = self.parse_expression()
            variants.append((v, val))
            self._skip_newlines()
        self._expect_terminator(("END", "ENUM"))
        return ast.EnumDef(name, variants, line)

    def parse_try(self):
        line = self._cur().line
        body = self.parse_block({("CATCH",), ("FINALLY",), ("END", "TRY")})
        catches = []
        while self._at_kw("CATCH"):
            self._advance()
            name = None
            type_filter = None
            if self._at(TokenType.IDENT):
                name = self._advance().value
                if self._at_kw("AS"):
                    self._advance()
                    type_filter = self.parse_type_name()
            elif self._at_kw("AS"):
                self._advance()
                type_filter = self.parse_type_name()
            cbody = self.parse_block({("CATCH",), ("FINALLY",), ("END", "TRY")})
            catches.append((name, type_filter, cbody))
        finally_body = None
        if self._at_kw("FINALLY"):
            self._advance()
            finally_body = self.parse_block({("END", "TRY")})
        self._expect_terminator(("END", "TRY"))
        return ast.TryStmt(body, catches, finally_body, line)

    def parse_import(self):
        line = self._cur().line
        if self._at_kw("FROM"):
            self._advance()
            mod = self._expect(TokenType.IDENT, "module name").value
            self._expect_kw("IMPORT", "IMPORT")
            names = []
            if self._at(TokenType.STAR):
                self._advance()
                names = ["*"]
            else:
                names.append(self._expect(TokenType.IDENT).value)
                while self._at(TokenType.COMMA):
                    self._advance()
                    names.append(self._expect(TokenType.IDENT).value)
            alias = None
            return ast.ImportStmt(mod, names, alias, line)
        mod = self._expect(TokenType.IDENT, "module name").value
        alias = None
        names = []
        if self._at_kw("AS"):
            self._advance()
            alias = self._expect(TokenType.IDENT).value
        elif self._at(TokenType.COLON) or self._at(TokenType.DOT):
            self._advance()
            names.append(self._expect(TokenType.IDENT).value)
            while self._at(TokenType.DOT):
                self._advance()
                names.append(self._expect(TokenType.IDENT).value)
        return ast.ImportStmt(mod, names, alias, line)

    def parse_export(self):
        line = self._cur().line
        names = [self._expect(TokenType.IDENT).value]
        while self._at(TokenType.COMMA):
            self._advance()
            names.append(self._expect(TokenType.IDENT).value)
        return ast.ExportStmt(names, line)

    def parse_goto(self):
        line = self._cur().line
        t = self._advance()
        if t.type == TokenType.INT:
            return ast.GotoStmt(str(t.value), line)
        if t.type == TokenType.IDENT:
            return ast.GotoStmt(t.value, line)
        raise ParseError("GOTO expects a label", t.line, t.col)

    def parse_gosub(self):
        line = self._cur().line
        t = self._advance()
        if t.type == TokenType.INT:
            return ast.GosubStmt(str(t.value), line)
        if t.type == TokenType.IDENT:
            return ast.GosubStmt(t.value, line)
        raise ParseError("GOSUB expects a label", t.line, t.col)

    def parse_data(self):
        line = self._cur().line
        vals = [self.parse_data_value()]
        while self._at(TokenType.COMMA):
            self._advance()
            vals.append(self.parse_data_value())
        return ast.DataStmt(vals, line)

    def parse_data_value(self):
        if self._at(TokenType.STRING):
            return ast.StrLit(self._advance().value)
        return self.parse_expression()

    def parse_read(self):
        line = self._cur().line
        vars_ = [self.parse_postfix()]
        while self._at(TokenType.COMMA):
            self._advance()
            vars_.append(self.parse_postfix())
        return ast.ReadStmt(vars_, line)

    def parse_restore(self):
        return ast.RestoreStmt(self._cur().line)

    def parse_randomize(self):
        line = self._cur().line
        seed = None
        if not (self._at(TokenType.NEWLINE) or self._at(TokenType.COLON)
                or self._at(TokenType.EOF) or self._at(TokenType.SEMI)):
            seed = self.parse_expression()
        return ast.RandomizeStmt(seed, line)

    def parse_option(self):
        line = self._cur().line
        self._expect_kw("BASE", "BASE")
        base = self.parse_expression()
        return ast.OptionBaseStmt(base, line)

    def parse_with(self):
        line = self._cur().line
        expr = self.parse_expression()
        as_name = None
        if self._at_kw("AS"):
            self._advance()
            as_name = self._expect(TokenType.IDENT).value
        body = self.parse_block({("END", "WITH")})
        self._expect_terminator(("END", "WITH"))
        return ast.WithStmt(expr, as_name, body, line)

    def parse_chan(self):
        # chan(TYPE, size)  -> we treat as expression that builds a channel
        # In statement position, this is just an expression statement.
        expr = self.parse_expression()
        return ast.ExprStmt(expr, self._cur().line)

    def parse_spawn(self):
        line = self._cur().line
        expr = self.parse_expression()
        return ast.SpawnStmt(expr, line)

    # ---- block parsing ----------------------------------------------------
    def parse_block(self, terminators):
        """Parse statements until one of the terminator sequences is reached.
        terminators: set of tuples of keyword names. e.g. {("END","IF"), ("ELSE",)}.
        Does not consume the terminator."""
        body = []
        self._skip_sep()
        self._skip_newlines()
        while True:
            if self._match_terminator(terminators):
                break
            if self._at(TokenType.EOF):
                raise ParseError("Unexpected end of input — expected " +
                                 _fmt_terms(terminators),
                                 self._cur().line, self._cur().col)
            stmt = self.parse_statement()
            if stmt is not None:
                if stmt.kind == "label":
                    self.labels[stmt.name] = len(body)
                else:
                    body.append(stmt)
            self._skip_sep()
            self._skip_newlines()
        return body

    def _match_terminator(self, terminators) -> bool:
        for seq in terminators:
            if self._peek_seq(seq):
                return True
        return False

    def _peek_seq(self, seq) -> bool:
        for off, kw in enumerate(seq):
            t = self._peek(off)
            if not (t.type == TokenType.KEYWORD and t.kw == kw):
                return False
        return True

    def _expect_terminator(self, *seqs):
        for seq in seqs:
            if self._peek_seq(seq):
                for _ in seq:
                    self._advance()
                return
        raise ParseError("Expected " + _fmt_terms(set(seqs)),
                         self._cur().line, self._cur().col)

    # ---- patterns ---------------------------------------------------------
    def parse_pattern(self):
        t = self._cur()
        if t.type == TokenType.INT:
            self._advance()
            if self._at(TokenType.DOTDOT) or self._at(TokenType.DOTDOTEQ):
                incl = self._at(TokenType.DOTDOTEQ)
                self._advance()
                hi = self.parse_expression()
                return ast.RangePat(ast.IntLit(t.value), hi, incl)
            return ast.LitPat(t.value)
        if t.type == TokenType.FLOAT:
            self._advance(); return ast.LitPat(t.value)
        if t.type == TokenType.STRING:
            self._advance(); return ast.LitPat(t.value)
        if t.type == TokenType.KEYWORD and t.kw in ("TRUE", "FALSE", "NONE"):
            self._advance()
            return ast.LitPat(True if t.kw == "TRUE" else (False if t.kw == "FALSE" else None))
        if t.type == TokenType.IDENT:
            name = self._advance().value
            if name == "_":
                return ast.WildcardPat()
            if self._at_kw("IS"):
                self._advance()
                type_name = self.parse_type_name()
                return ast.IsPat(name, type_name)
            if self._at(TokenType.LPAREN):
                # constructor pattern Name(args)
                self._advance()
                sub = [self.parse_pattern()]
                while self._at(TokenType.COMMA):
                    self._advance()
                    sub.append(self.parse_pattern())
                self._expect(TokenType.RPAREN, "')'")
                return ast.CtorPat(name, sub)
            return ast.BindPat(name)
        if t.type == TokenType.LPAREN or t.type == TokenType.LBRACKET:
            close = TokenType.RPAREN if t.type == TokenType.LPAREN else TokenType.RBRACKET
            self._advance()
            subs = []
            if not self._at(close):
                subs.append(self.parse_pattern())
                while self._at(TokenType.COMMA):
                    self._advance()
                    subs.append(self.parse_pattern())
            self._expect(close, "matching bracket")
            return ast.TuplePat(subs)
        raise ParseError("Invalid pattern", t.line, t.col)

    def parse_type_name(self):
        t = self._cur()
        if t.type == TokenType.IDENT:
            self._advance()
            return t.value
        if t.type == TokenType.KEYWORD:
            self._advance(); return t.kw.lower()
        raise ParseError("Expected type name", t.line, t.col)

    # ---- types ------------------------------------------------------------
    def parse_type(self):
        # int / float / string / bool / list[T] / dict[K,V] / T? / T... / custom
        t = self._cur()
        if t.type == TokenType.KEYWORD and t.kw in ("INT", "STRING", "BOOL", "NONE", "FLOAT"):
            # these aren't keywords actually; IDENT path handles
            pass
        name = self.parse_type_name()
        if self._at(TokenType.LT):
            self._advance()
            args = [self.parse_type()]
            while self._at(TokenType.COMMA):
                self._advance(); args.append(self.parse_type())
            self._expect(TokenType.GT, "'>'")
        if self._at(TokenType.QMARK):
            self._advance()
        return name

    # ---- expressions (Pratt precedence) -----------------------------------
    def parse_expression(self):
        return self.parse_ternary()

    def parse_ternary(self):
        cond = self.parse_nullish()
        if self._at(TokenType.QMARK):
            self._advance()
            then = self.parse_ternary()
            self._expect(TokenType.COLON, "':'")
            els = self.parse_ternary()
            return ast.Ternary(cond, then, els, cond.line if hasattr(cond, 'line') else 0)
        # 'if' expression form:  expr IF cond ELSE expr  (Python-ish) — but IF is keyword-stmt.
        return cond

    def parse_nullish(self):
        left = self.parse_or()
        while self._at(TokenType.QQ):
            self._advance()
            right = self.parse_or()
            left = ast.Nullish(left, right, getattr(left, 'line', 0))
        return left

    def parse_or(self):
        left = self.parse_and()
        while self._at(TokenType.PIPEPIPE) or self._at_kw("OR", "XOR"):
            op = "or"
            if self._at_kw("XOR"): op = "xor"
            self._advance()
            right = self.parse_and()
            left = ast.LogicalOp(op, left, right, getattr(left, 'line', 0))
        return left

    def parse_and(self):
        left = self.parse_equality()
        while self._at(TokenType.AMPAMP) or self._at_kw("AND"):
            self._advance()
            right = self.parse_equality()
            left = ast.LogicalOp("and", left, right, getattr(left, 'line', 0))
        return left

    def parse_equality(self):
        left = self.parse_comparison()
        while self._at(TokenType.EQEQ) or self._at(TokenType.NEQ) or self._at(TokenType.EQ):
            op = "==" if self._cur().type == TokenType.EQEQ else ("!=" if self._cur().type == TokenType.NEQ else "==")
            self._advance()
            right = self.parse_comparison()
            left = ast.BinOp(op, left, right, getattr(left, 'line', 0))
        return left

    def parse_comparison(self):
        left = self.parse_range()
        while self._at(TokenType.LT) or self._at(TokenType.GT) or self._at(TokenType.LE) or self._at(TokenType.GE) or self._at_kw("IN", "IS"):
            if self._at_kw("IN"):
                self._advance()
                right = self.parse_range()
                left = ast.BinOp("in", left, right, getattr(left, 'line', 0))
            elif self._at_kw("IS"):
                self._advance()
                # is None / is Type
                if self._at_kw("NONE"):
                    self._advance()
                    left = ast.BinOp("is", left, ast.NoneLit(), getattr(left, 'line', 0))
                else:
                    nm = self.parse_type_name()
                    left = ast.BinOp("is", left, ast.Ident(nm), getattr(left, 'line', 0))
            else:
                op = self._cur().value
                self._advance()
                right = self.parse_range()
                left = ast.BinOp(op, left, right, getattr(left, 'line', 0))
        return left

    def parse_range(self):
        left = self.parse_additive()
        if self._at(TokenType.DOTDOT) or self._at(TokenType.DOTDOTEQ):
            incl = self._at(TokenType.DOTDOTEQ)
            self._advance()
            right = self.parse_additive()
            left = ast.RangeLit(left, right, incl, getattr(left, 'line', 0))
        return left

    def parse_additive(self):
        left = self.parse_multiplicative()
        while self._at(TokenType.PLUS) or self._at(TokenType.MINUS):
            op = self._cur().value
            self._advance()
            right = self.parse_multiplicative()
            left = ast.BinOp(op, left, right, getattr(left, 'line', 0))
        return left

    def parse_multiplicative(self):
        left = self.parse_unary()
        while (self._at(TokenType.STAR) or self._at(TokenType.SLASH)
               or self._at(TokenType.PERCENT) or self._at_kw("MOD", "DIV", "SHL", "SHR")):
            if self._at_kw("MOD"): op = "mod"
            elif self._at_kw("DIV"): op = "div"
            elif self._at_kw("SHL"): op = "shl"
            elif self._at_kw("SHR"): op = "shr"
            else: op = self._cur().value
            self._advance()
            right = self.parse_unary()
            left = ast.BinOp(op, left, right, getattr(left, 'line', 0))
        return left

    def parse_unary(self):
        if self._at(TokenType.MINUS) or self._at(TokenType.PLUS):
            op = self._cur().value
            self._advance()
            operand = self.parse_unary()
            return ast.UnaryOp(op, operand, self._cur().line)
        if self._at_kw("NOT"):
            self._advance()
            operand = self.parse_unary()
            return ast.UnaryOp("not", operand, self._cur().line)
        if self._at(TokenType.SEND):
            # <-ch  receive
            self._advance()
            chan = self.parse_unary()
            return ast.ChanRecv(chan, self._cur().line)
        if self._at_kw("AWAIT"):
            self._advance()
            e = self.parse_unary()
            return ast.AwaitExpr(e, self._cur().line)
        if self._at_kw("SPAWN") or self._at_kw("GO"):
            self._advance()
            e = self.parse_unary()
            return ast.SpawnStmt(e, self._cur().line)  # used as expr too via SpawnStmt? make expr
        if self._at(TokenType.DOTDOTDOT):
            self._advance()
            e = self.parse_unary()
            return ast.Spread(e, self._cur().line)
        return self.parse_power()

    def parse_power(self):
        left = self.parse_postfix()
        if self._at(TokenType.CARET):
            self._advance()
            right = self.parse_unary()  # right-assoc, and unary allows -x
            left = ast.BinOp("^", left, right, getattr(left, 'line', 0))
        return left

    def _member_name(self):
        t = self._cur()
        if t.type == TokenType.IDENT:
            self._advance(); return t.value
        if t.type == TokenType.KEYWORD:
            self._advance(); return t.value.lower()
        raise ParseError(f"Expected member name, got {t.type.name} ({t.value!r})", t.line, t.col)

    def parse_postfix(self):
        expr = self.parse_primary()
        while True:
            if self._at(TokenType.LPAREN):
                args, kwargs = self.parse_call_args_raw()
                expr = ast.Call(expr, args, kwargs, self._cur().line)
            elif self._at(TokenType.LBRACKET):
                self._advance()
                # slice or index
                start = stop = step = None
                is_slice = False
                if not self._at(TokenType.COLON):
                    start = self.parse_expression()
                if self._at(TokenType.COLON):
                    is_slice = True
                    self._advance()
                    if not (self._at(TokenType.COLON) or self._at(TokenType.RBRACKET)):
                        stop = self.parse_expression()
                    if self._at(TokenType.COLON):
                        self._advance()
                        if not self._at(TokenType.RBRACKET):
                            step = self.parse_expression()
                self._expect(TokenType.RBRACKET, "']'")
                if is_slice:
                    expr = ast.Slice(expr, start, stop, step, self._cur().line)
                else:
                    expr = ast.Index(expr, start, self._cur().line)
            elif self._at(TokenType.DOT):
                self._advance()
                name = self._member_name()
                expr = ast.Member(expr, name, self._cur().line)
            elif self._at(TokenType.QDOT):
                self._advance()
                name = self._member_name()
                expr = ast.OptMember(expr, name, self._cur().line)
            elif self._at(TokenType.COLONCOLON):
                self._advance()
                name = self._expect(TokenType.IDENT).value
                expr = ast.Member(expr, name, self._cur().line)
            elif self._at(TokenType.SEND):
                # ch <- value  (channel send) — only valid as statement, but parse here
                self._advance()
                val = self.parse_expression()
                expr = ast.SendStmt(expr, val, self._cur().line)
            else:
                break
        return expr

    def parse_call_args(self):
        """Used by decorators: returns (args, kwargs) assuming LPAREN at cur."""
        return self.parse_call_args_raw()

    def parse_call_args_raw(self):
        self._expect(TokenType.LPAREN, "'('")
        args = []
        kwargs = {}
        while not self._at(TokenType.RPAREN):
            if self._at(TokenType.IDENT) and self._peek(1).type == TokenType.EQ:
                kname = self._advance().value
                self._advance()  # =
                kval = self.parse_expression()
                kwargs[kname] = kval
            elif self._at(TokenType.DOTDOTDOT):
                self._advance()
                args.append(ast.Spread(self.parse_expression(), self._cur().line))
            else:
                args.append(self.parse_expression())
            if self._at(TokenType.COMMA):
                self._advance(); continue
            break
        self._expect(TokenType.RPAREN, "')'")
        return args, kwargs

    def _is_arrow_lambda_ahead(self):
        """Detect `( params ) => body` so paren-grouping vs arrow-lambda is unambiguous."""
        if not self._at(TokenType.LPAREN):
            return False
        j = self.i + 1
        n = len(self.toks)
        if j < n and self.toks[j].type == TokenType.RPAREN:
            return j + 1 < n and self.toks[j + 1].type == TokenType.ARROW
        while j < n:
            t = self.toks[j]
            if t.type != TokenType.IDENT:
                return False
            j += 1
            # optional : type annotation
            if j < n and self.toks[j].type == TokenType.COLON:
                j += 1
                while j < n and self.toks[j].type not in (
                        TokenType.COMMA, TokenType.RPAREN, TokenType.EQ,
                        TokenType.ARROW):
                    j += 1
            # optional = default value (scan balanced)
            if j < n and self.toks[j].type == TokenType.EQ:
                j += 1
                depth = 0
                while j < n:
                    tt = self.toks[j].type
                    if tt in (TokenType.LPAREN, TokenType.LBRACKET, TokenType.LBRACE):
                        depth += 1
                    elif tt in (TokenType.RPAREN, TokenType.RBRACKET, TokenType.RBRACE):
                        if depth == 0:
                            break
                        depth -= 1
                    elif tt == TokenType.COMMA and depth == 0:
                        break
                    j += 1
            if j < n and self.toks[j].type == TokenType.COMMA:
                j += 1
                continue
            break
        if j < n and self.toks[j].type == TokenType.RPAREN:
            return j + 1 < n and self.toks[j + 1].type == TokenType.ARROW
        return False

    def parse_primary(self):
        t = self._cur()
        # keywords usable as expression values
        if t.type == TokenType.INT:
            self._advance(); return ast.IntLit(t.value, t.line)
        if t.type == TokenType.FLOAT:
            self._advance(); return ast.FloatLit(t.value, t.line)
        if t.type == TokenType.STRING:
            self._advance(); return ast.StrLit(t.value, t.line)
        if t.type == TokenType.REGEX:
            self._advance(); return ast.RegexLit(t.value, t.line)
        if t.type == TokenType.FSTRING:
            return self.parse_fstring()
        if t.type == TokenType.KEYWORD:
            if t.kw == "TRUE":
                self._advance(); return ast.BoolLit(True, t.line)
            if t.kw == "FALSE":
                self._advance(); return ast.BoolLit(False, t.line)
            if t.kw == "NONE":
                self._advance(); return ast.NoneLit(t.line)
            if t.kw == "ME":
                self._advance(); return ast.Ident("me", t.line)
            if t.kw == "NEW":
                self._advance()
                callee = self.parse_postfix_no_call()
                args, kwargs = ({}, {})
                if self._at(TokenType.LPAREN):
                    args, kwargs = self.parse_call_args_raw()
                return ast.Call(ast.Ident("__new__"), [callee] + args, kwargs, t.line)
            if t.kw == "LAMBDA":
                return self.parse_lambda()
            if t.kw == "CHAN":
                self._advance()
                self._expect(TokenType.LPAREN, "'('")
                # chan(TYPE, size) — TYPE ignored, size optional
                self.parse_type()
                size = ast.IntLit(0)
                if self._at(TokenType.COMMA):
                    self._advance()
                    size = self.parse_expression()
                self._expect(TokenType.RPAREN, "')'")
                return ast.Call(ast.Ident("__chan__"), [size], {}, t.line)
            if t.kw == "MATCH":
                # match expression with braces
                self._advance()
                scrut = self.parse_expression()
                self._expect(TokenType.LBRACE, "'{'")
                arms = []
                self._skip_newlines()
                while not self._at(TokenType.RBRACE):
                    self._expect_kw("CASE", "CASE")
                    if self._at_kw("ELSE"):
                        self._advance()
                        self._expect(TokenType.ARROW, "'=>'")
                        body = self.parse_expression()
                        arms.append((ast.WildcardPat(), None, body))
                    else:
                        pats = [self.parse_pattern()]
                        while self._at(TokenType.COMMA):
                            self._advance(); pats.append(self.parse_pattern())
                        guard = None
                        if self._at_kw("IF"):
                            self._advance(); guard = self.parse_expression()
                        self._expect(TokenType.ARROW, "'=>'")
                        body = self.parse_expression()
                        arms.append((ast.OrPat(pats) if len(pats) > 1 else pats[0], guard, body))
                    if self._at(TokenType.COMMA): self._advance()
                    self._skip_newlines()
                self._expect(TokenType.RBRACE, "'}'")
                return ast.MatchExpr(scrut, arms, t.line)
            if t.kw == "AWAIT":
                self._advance()
                e = self.parse_unary()
                return ast.AwaitExpr(e, t.line)
            if t.kw == "ASYNC":
                # async lambda: async (params) => body  OR async function() ... end function (stmt)
                self._advance()
                if self._at_kw("FUNCTION", "FN"):
                    return self.parse_function()  # statement form; rare in expr
                return self.parse_lambda(is_async=True)
            # 'fn' as anon function: fn(params) => body
            if t.kw == "FN":
                self._advance()
                return self.parse_lambda()
            if t.kw == "DEF":
                self._advance()
                return self.parse_lambda()
            if t.kw == "PRINT":
                # allow print(...) as a callable in expression position
                self._advance()
                return ast.Ident("print", t.line)
            if t.kw == "INPUT":
                self._advance()
                return ast.Ident("input", t.line)
            if t.kw == "TYPE":
                self._advance()
                return ast.Ident("type", t.line)
            raise ParseError(f"Unexpected keyword {t.kw} in expression", t.line, t.col)
        if t.type == TokenType.IDENT:
            if self._peek(1).type == TokenType.ARROW:
                return self.parse_lambda()
            self._advance()
            return ast.Ident(t.value, t.line)
        if t.type == TokenType.LPAREN:
            if self._is_arrow_lambda_ahead():
                return self.parse_lambda()
            self._advance()
            e = self.parse_expression()
            if self._at(TokenType.COMMA):
                items = [e]
                while self._at(TokenType.COMMA):
                    self._advance()
                    items.append(self.parse_expression())
                self._expect(TokenType.RPAREN, "')'")
                return ast.TupleLit(items, t.line)
            self._expect(TokenType.RPAREN, "')'")
            return e
        if t.type == TokenType.LBRACKET:
            return self.parse_list_or_comp()
        if t.type == TokenType.LBRACE:
            return self.parse_dict_or_comp()
        raise ParseError(f"Unexpected token {t.type.name} ({t.value!r})", t.line, t.col)

    def parse_postfix_no_call(self):
        """Like parse_postfix but stops at LPAREN (for NEW Type(...))."""
        expr = self.parse_primary()
        while True:
            if self._at(TokenType.LBRACKET):
                self._advance()
                start = stop = step = None; is_slice = False
                if not self._at(TokenType.COLON):
                    start = self.parse_expression()
                if self._at(TokenType.COLON):
                    is_slice = True; self._advance()
                    if not (self._at(TokenType.COLON) or self._at(TokenType.RBRACKET)):
                        stop = self.parse_expression()
                    if self._at(TokenType.COLON):
                        self._advance()
                        if not self._at(TokenType.RBRACKET):
                            step = self.parse_expression()
                self._expect(TokenType.RBRACKET, "']'")
                expr = ast.Slice(expr, start, stop, step, self._cur().line) if is_slice else ast.Index(expr, start, self._cur().line)
            elif self._at(TokenType.DOT):
                self._advance()
                name = self._member_name()
                expr = ast.Member(expr, name, self._cur().line)
            else:
                break
        return expr

    def parse_list_or_comp(self):
        line = self._cur().line
        self._expect(TokenType.LBRACKET, "'['")
        if self._at(TokenType.RBRACKET):
            self._advance()
            return ast.ListLit([], line)
        first = self.parse_expression()
        if self._at_kw("FOR"):
            # comprehension
            clauses = self.parse_comp_clauses()
            self._expect(TokenType.RBRACKET, "']'")
            return ast.Comprehension("list", first, clauses, line)
        items = [first]
        while self._at(TokenType.COMMA):
            self._advance()
            if self._at(TokenType.RBRACKET):
                break  # trailing comma
            items.append(self.parse_expression())
        self._expect(TokenType.RBRACKET, "']'")
        return ast.ListLit(items, line)

    def parse_dict_or_comp(self):
        line = self._cur().line
        self._expect(TokenType.LBRACE, "'{'")
        if self._at(TokenType.RBRACE):
            self._advance()
            return ast.DictLit([], line)
        first_key = self.parse_expression()
        self._expect(TokenType.COLON, "':'")
        first_val = self.parse_expression()
        if self._at_kw("FOR"):
            clauses = self.parse_comp_clauses()
            self._expect(TokenType.RBRACE, "'}'")
            return ast.Comprehension("dict", None, clauses, line, key=first_key, value=first_val)
        pairs = [(first_key, first_val)]
        while self._at(TokenType.COMMA):
            self._advance()
            if self._at(TokenType.RBRACE):
                break
            k = self.parse_expression()
            self._expect(TokenType.COLON, "':'")
            v = self.parse_expression()
            pairs.append((k, v))
        self._expect(TokenType.RBRACE, "'}'")
        return ast.DictLit(pairs, line)

    def parse_comp_clauses(self):
        clauses = []
        while self._at_kw("FOR") or self._at_kw("IF"):
            if self._at_kw("IF"):
                self._advance()
                cond = self.parse_expression()
                clauses.append(("if", cond))
            else:
                self._advance()
                name = self._expect(TokenType.IDENT, "loop variable").value
                self._expect_kw("IN", "IN")
                it = self.parse_expression()
                clauses.append(("for", name, it))
        return clauses

    def parse_lambda(self, is_async=False):
        line = self._cur().line
        params = []
        if self._at(TokenType.LPAREN):
            params = self.parse_param_list()
        elif self._at(TokenType.IDENT):
            params.append(self._parse_one_param())
            while self._at(TokenType.COMMA):
                self._advance()
                params.append(self._parse_one_param())
        self._expect(TokenType.ARROW, "'=>'")
        if self._at(TokenType.NEWLINE) or self._at(TokenType.LBRACE):
            # block body
            if self._at(TokenType.LBRACE):
                self._advance()
                body = []
                while not self._at(TokenType.RBRACE):
                    if self._at(TokenType.EOF):
                        raise ParseError("Unterminated lambda block", line, 0)
                    self._skip_sep()
                    self._skip_newlines()
                    if self._at(TokenType.RBRACE):
                        break
                    s = self.parse_statement()
                    if s is not None: body.append(s)
                    self._skip_sep()
                self._expect(TokenType.RBRACE, "'}'")
                return ast.Lambda(params, body, expr_form=False, line=line)
            body = self.parse_block({("END", "FUNCTION"), ("END", "FN")})
            self._expect_terminator(("END", "FUNCTION"), ("END", "FN"))
            return ast.Lambda(params, body, expr_form=False, line=line)
        body = self.parse_expression()
        return ast.Lambda(params, body, expr_form=True, line=line)

    def parse_fstring(self):
        t = self._advance()
        segs = t.value  # list of ('lit', text) or ('expr', src, fmt)
        out = []
        for seg in segs:
            if seg[0] == "lit":
                out.append(("lit", seg[1]))
            else:
                src, fmt = seg[1], seg[2]
                # re-lex+parse the expression
                from .lexer import Lexer
                sub_tokens = Lexer(src).tokenize()
                expr = Parser(sub_tokens).parse_expression()
                out.append(("expr", expr, fmt))
        return ast.FStringLit(out, t.line)


def _fmt_terms(terminators):
    return " or ".join(" ".join(seq) for seq in terminators)


def _contains_yield(body) -> bool:
    """Recursively scan a statement list for a YIELD statement/expression."""
    for node in body:
        if _scan_yield(node):
            return True
    return False


def _scan_yield(node):
    if node is None:
        return False
    k = getattr(node, "kind", "")
    if k == "yield":
        return True
    if k == "func" or k == "sub" or k == "class":
        return False  # nested defs don't count
    # walk common fields
    for attr in ("body", "then", "els", "cond", "value", "expr", "operand",
                 "left", "right", "callee", "obj", "start", "end", "step",
                 "iter", "scrutinee", "call"):
        v = getattr(node, attr, None)
        if isinstance(v, list):
            for x in v:
                if _scan_yield(x): return True
        elif isinstance(v, tuple):
            for x in v:
                if isinstance(x, list):
                    for y in x:
                        if _scan_yield(y): return True
                elif _scan_yield(x): return True
        elif _scan_yield(v):
            return True
    return False


# ---- pattern AST nodes (kept here to avoid cluttering ast_nodes) ----------
# We attach them to ast_nodes for convenience.
class _PatBase:
    kind = "pattern"

class LitPat(_PatBase):
    def __init__(self, value): self.value = value; self.kind = "litpat"
class BindPat(_PatBase):
    def __init__(self, name): self.name = name; self.kind = "bindpat"
class WildcardPat(_PatBase):
    kind = "wildpat"
class TuplePat(_PatBase):
    def __init__(self, subs): self.subs = subs; self.kind = "tuplepat"
class CtorPat(_PatBase):
    def __init__(self, name, args): self.name = name; self.args = args; self.kind = "ctorpat"
class IsPat(_PatBase):
    def __init__(self, name, type_name): self.name = name; self.type_name = type_name; self.kind = "ispat"
class RangePat(_PatBase):
    def __init__(self, lo, hi, inclusive): self.lo = lo; self.hi = hi; self.inclusive = inclusive; self.kind = "rangepat"
class OrPat(_PatBase):
    def __init__(self, subs): self.subs = subs; self.kind = "orpat"

# attach to ast namespace so interpreter can import
ast.LitPat = LitPat
ast.BindPat = BindPat
ast.WildcardPat = WildcardPat
ast.TuplePat = TuplePat
ast.CtorPat = CtorPat
ast.IsPat = IsPat
ast.RangePat = RangePat
ast.OrPat = OrPat


# ---- keyword dispatch table ----------------------------------------------
Parser._kw_handlers = {
    "PRINT": Parser.parse_print,
    "INPUT": Parser.parse_input,
    "LET": Parser.parse_let,
    "VAR": Parser.parse_let,
    "CONST": Parser.parse_let,
    "DIM": Parser.parse_dim,
    "IF": Parser.parse_if,
    "FOR": Parser.parse_for,
    "WHILE": Parser.parse_while,
    "DO": Parser.parse_do,
    "REPEAT": Parser.parse_repeat,
    "SELECT": Parser.parse_select,
    "MATCH": Parser.parse_match,
    "FUNCTION": Parser.parse_function,
    "FN": Parser.parse_function,
    "SUB": Parser.parse_function,
    "DEF": Parser.parse_function,
    "RETURN": lambda self: ast.ReturnStmt(
        (self.parse_expression() if not (self._at(TokenType.NEWLINE) or self._at(TokenType.COLON)
                                         or self._at(TokenType.EOF) or self._at(TokenType.SEMI)) else None),
        self._cur().line),
    "BREAK": lambda self: ast.BreakStmt(self._cur().line),
    "CONTINUE": lambda self: ast.ContinueStmt(self._cur().line),
    "EXIT": lambda self: (self._advance() if self._at_kw("FOR", "WHILE", "DO", "SUB", "FUNCTION") else None,
                          ast.BreakStmt(self._cur().line)),
    "PASS": lambda self: ast.PassStmt(self._cur().line),
    "STOP": lambda self: ast.StopStmt(self._cur().line),
    "END": lambda self: (
        (self._advance() if self._at_kw("IF", "FUNCTION", "SUB", "SELECT", "CLASS",
                                        "FOR", "WHILE", "TRY", "MATCH", "ENUM",
                                        "FN", "DEF", "WITH") else None),
        ast.StopStmt(self._cur().line)),
    "CLASS": Parser.parse_class,
    "ENUM": Parser.parse_enum,
    "TRY": Parser.parse_try,
    "IMPORT": Parser.parse_import,
    "FROM": Parser.parse_import,
    "EXPORT": Parser.parse_export,
    "GOTO": Parser.parse_goto,
    "GOSUB": Parser.parse_gosub,
    "DATA": Parser.parse_data,
    "READ": Parser.parse_read,
    "RESTORE": Parser.parse_restore,
    "RANDOMIZE": Parser.parse_randomize,
    "OPTION": Parser.parse_option,
    "WITH": Parser.parse_with,
    "CHAN": Parser.parse_chan,
    "SPAWN": Parser.parse_spawn,
    "GO": Parser.parse_spawn,
    "ASYNC": Parser.parse_function,
    "ON": lambda self: (self._advance(),  # ON ERROR GOTO / ON expr GOTO
                        Parser.parse_on(self)),
}

def _parse_on(self):
    # ON ERROR GOTO label  /  ON ERROR RESUME NEXT  /  ON expr GOTO a, b, ...
    if self._at_kw("ERROR"):
        self._advance()
        # ON ERROR GOTO label  or ON ERROR RESUME NEXT
        if self._at_kw("RESUME"):
            self._advance()
            self._expect_kw("NEXT", "NEXT")
            return ast.PassStmt(self._cur().line)
        if self._at_kw("GOTO"):
            self._advance()
            t = self._advance()
            return ast.ExprStmt(ast.Ident("__on_error__"), t.line)
        return ast.PassStmt(self._cur().line)
    # ON expr GOTO a, b, ...
    expr = self.parse_expression()
    if self._at_kw("GOTO"):
        self._advance()
        targets = []
        t = self._advance()
        targets.append(str(t.value) if t.type in (TokenType.INT, TokenType.IDENT) else None)
        while self._at(TokenType.COMMA):
            self._advance()
            t = self._advance()
            targets.append(str(t.value))
        # encode as a switch-like: we wrap in a SelectStmt with int cases
        cases = []
        for idx, tgt in enumerate(targets):
            cases.append(([ast.IntLit(idx+1)], None, [ast.GotoStmt(tgt, self._cur().line)]))
        return ast.SelectStmt(expr, cases, None, self._cur().line)
    raise ParseError("Expected GOTO after ON", self._cur().line, self._cur().col)
Parser.parse_on = _parse_on
