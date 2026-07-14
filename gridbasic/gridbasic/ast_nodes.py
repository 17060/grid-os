"""Abstract Syntax Tree node definitions for GridBasic.

Every node is a tiny dataclass-like class with a ``kind`` attribute so the
interpreter can dispatch with a single match. Keeping them simple makes the
tree-walking evaluator easy to read and extend.
"""

from __future__ import annotations


class Node:
    kind: str = "Node"
    __slots__ = ()


# ---- expressions ----------------------------------------------------------
class IntLit(Node):
    __slots__ = ("value", "line")
    kind = "int"
    def __init__(self, value, line=0): self.value = value; self.line = line

class FloatLit(Node):
    __slots__ = ("value", "line")
    kind = "float"
    def __init__(self, value, line=0): self.value = value; self.line = line

class StrLit(Node):
    __slots__ = ("value", "line")
    kind = "str"
    def __init__(self, value, line=0): self.value = value; self.line = line

class BoolLit(Node):
    __slots__ = ("value", "line")
    kind = "bool"
    def __init__(self, value, line=0): self.value = value; self.line = line

class NoneLit(Node):
    __slots__ = ("line")
    kind = "none"
    def __init__(self, line=0): self.line = line

class RegexLit(Node):
    __slots__ = ("value", "line")
    kind = "regex"
    def __init__(self, value, line=0): self.value = value; self.line = line

class FStringLit(Node):
    """segments: list of ('lit', text) or ('expr', ast, fmt_or_None)"""
    __slots__ = ("segments", "line")
    kind = "fstring"
    def __init__(self, segments, line=0): self.segments = segments; self.line = line

class Ident(Node):
    __slots__ = ("name", "line")
    kind = "ident"
    def __init__(self, name, line=0): self.name = name; self.line = line

class ListLit(Node):
    __slots__ = ("items", "line")
    kind = "list"
    def __init__(self, items, line=0): self.items = items; self.line = line

class DictLit(Node):
    __slots__ = ("pairs", "line")
    kind = "dict"
    def __init__(self, pairs, line=0): self.pairs = pairs; self.line = line

class TupleLit(Node):
    __slots__ = ("items", "line")
    kind = "tuple"
    def __init__(self, items, line=0): self.items = items; self.line = line

class RangeLit(Node):
    __slots__ = ("start", "end", "inclusive", "line")
    kind = "range"
    def __init__(self, start, end, inclusive, line=0):
        self.start = start; self.end = end; self.inclusive = inclusive; self.line = line

class Comprehension(Node):
    """List or dict comprehension. For list: expr is element expr.
    For dict: key/value. clauses is a list of ('for', name, iter) or ('if', cond)."""
    __slots__ = ("flavor", "expr", "key", "value", "clauses", "line")
    kind = "comprehension"
    def __init__(self, flavor, expr, clauses, line=0, key=None, value=None):
        self.flavor = flavor  # 'list' or 'dict'
        self.expr = expr
        self.clauses = clauses
        self.key = key
        self.value = value
        self.line = line

class BinOp(Node):
    __slots__ = ("op", "left", "right", "line")
    kind = "binop"
    def __init__(self, op, left, right, line=0):
        self.op = op; self.left = left; self.right = right; self.line = line

class UnaryOp(Node):
    __slots__ = ("op", "operand", "line")
    kind = "unop"
    def __init__(self, op, operand, line=0):
        self.op = op; self.operand = operand; self.line = line

class LogicalOp(Node):
    __slots__ = ("op", "left", "right", "line")
    kind = "logical"
    def __init__(self, op, left, right, line=0):
        self.op = op; self.left = left; self.right = right; self.line = line

class Assign(Node):
    """Plain assignment target = expr."""
    __slots__ = ("target", "value", "line")
    kind = "assign"
    def __init__(self, target, value, line=0):
        self.target = target; self.value = value; self.line = line

class AssignOp(Node):
    """Compound assignment target op= value (e.g. x += 1)."""
    __slots__ = ("op", "target", "value", "line")
    kind = "assignop"
    def __init__(self, op, target, value, line=0):
        self.op = op; self.target = target; self.value = value; self.line = line

class Call(Node):
    __slots__ = ("callee", "args", "kwargs", "line")
    kind = "call"
    def __init__(self, callee, args, kwargs, line=0):
        self.callee = callee; self.args = args; self.kwargs = kwargs; self.line = line

class Index(Node):
    __slots__ = ("obj", "index", "line")
    kind = "index"
    def __init__(self, obj, index, line=0):
        self.obj = obj; self.index = index; self.line = line

class Slice(Node):
    __slots__ = ("obj", "start", "stop", "step", "line")
    kind = "slice"
    def __init__(self, obj, start, stop, step, line=0):
        self.obj = obj; self.start = start; self.stop = stop; self.step = step; self.line = line

class Member(Node):
    __slots__ = ("obj", "name", "line")
    kind = "member"
    def __init__(self, obj, name, line=0):
        self.obj = obj; self.name = name; self.line = line

class OptMember(Node):
    """Optional chaining a?.b"""
    __slots__ = ("obj", "name", "line")
    kind = "optmember"
    def __init__(self, obj, name, line=0):
        self.obj = obj; self.name = name; self.line = line

class Lambda(Node):
    __slots__ = ("params", "body", "expr_form", "line")
    kind = "lambda"
    def __init__(self, params, body, expr_form, line=0):
        self.params = params; self.body = body; self.expr_form = expr_form; self.line = line

class Ternary(Node):
    __slots__ = ("cond", "then", "els", "line")
    kind = "ternary"
    def __init__(self, cond, then, els, line=0):
        self.cond = cond; self.then = then; self.els = els; self.line = line

class Nullish(Node):
    __slots__ = ("left", "right", "line")
    kind = "nullish"
    def __init__(self, left, right, line=0):
        self.left = left; self.right = right; self.line = line

class Spread(Node):
    __slots__ = ("expr", "line")
    kind = "spread"
    def __init__(self, expr, line=0): self.expr = expr; self.line = line

class AwaitExpr(Node):
    __slots__ = ("expr", "line")
    kind = "await"
    def __init__(self, expr, line=0): self.expr = expr; self.line = line

class ChanRecv(Node):
    """Receive from channel: <-ch  (prefix)."""
    __slots__ = ("chan", "line")
    kind = "chanrecv"
    def __init__(self, chan, line=0): self.chan = chan; self.line = line

class MatchExpr(Node):
    """match expression form (returns a value)."""
    __slots__ = ("scrutinee", "arms", "line")
    kind = "matchexpr"
    def __init__(self, scrutinee, arms, line=0):
        self.scrutinee = scrutinee; self.arms = arms; self.line = line


# ---- statements -----------------------------------------------------------
class Program(Node):
    __slots__ = ("body", "labels")
    kind = "program"
    def __init__(self, body, labels):
        self.body = body; self.labels = labels  # labels: name -> stmt index

class LabelDecl(Node):
    __slots__ = ("name", "line")
    kind = "label"
    def __init__(self, name, line=0): self.name = name; self.line = line

class ExprStmt(Node):
    __slots__ = ("expr", "line")
    kind = "exprstmt"
    def __init__(self, expr, line=0): self.expr = expr; self.line = line

class PrintStmt(Node):
    __slots__ = ("parts", "newline", "using", "line")
    kind = "print"
    def __init__(self, parts, newline, using, line=0):
        self.parts = parts  # list of expr
        self.newline = newline
        self.using = using
        self.line = line

class InputStmt(Node):
    __slots__ = ("prompt", "var", "line")
    kind = "input"
    def __init__(self, prompt, var, line=0):
        self.prompt = prompt; self.var = var; self.line = line

class LetStmt(Node):
    __slots__ = ("name", "type_ann", "value", "is_const", "line")
    kind = "let"
    def __init__(self, name, type_ann, value, is_const, line=0):
        self.name = name; self.type_ann = type_ann; self.value = value
        self.is_const = is_const; self.line = line

class DestructureStmt(Node):
    __slots__ = ("pattern", "value", "flavor", "line")
    kind = "destructure"
    def __init__(self, pattern, value, flavor, line=0):
        self.pattern = pattern; self.value = value; self.flavor = flavor  # 'list'/'dict'
        self.line = line

class DimStmt(Node):
    __slots__ = ("name", "dims", "line")
    kind = "dim"
    def __init__(self, name, dims, line=0):
        self.name = name; self.dims = dims; self.line = line

class BlockStmt(Node):
    __slots__ = ("body", "line")
    kind = "block"
    def __init__(self, body, line=0): self.body = body; self.line = line

class IfStmt(Node):
    __slots__ = ("branches", "else_body", "line")
    kind = "if"
    def __init__(self, branches, else_body, line=0):
        # branches: list of (cond, body)
        self.branches = branches; self.else_body = else_body; self.line = line

class ForStmt(Node):
    __slots__ = ("var", "start", "end", "step", "body", "line")
    kind = "for"
    def __init__(self, var, start, end, step, body, line=0):
        self.var = var; self.start = start; self.end = end
        self.step = step; self.body = body; self.line = line

class ForInStmt(Node):
    __slots__ = ("var", "iter", "body", "line")
    kind = "forin"
    def __init__(self, var, iter, body, line=0):
        self.var = var; self.iter = iter; self.body = body; self.line = line

class WhileStmt(Node):
    __slots__ = ("cond", "body", "line")
    kind = "while"
    def __init__(self, cond, body, line=0):
        self.cond = cond; self.body = body; self.line = line

class RepeatStmt(Node):
    __slots__ = ("body", "cond", "line")
    kind = "repeat"
    def __init__(self, body, cond, line=0):
        self.body = body; self.cond = cond; self.line = line

class SelectStmt(Node):
    __slots__ = ("expr", "cases", "else_body", "line")
    kind = "select"
    def __init__(self, expr, cases, else_body, line=0):
        # cases: list of (list_of_case_exprs_or_None, body)
        self.expr = expr; self.cases = cases; self.else_body = else_body; self.line = line

class MatchStmt(Node):
    __slots__ = ("scrutinee", "arms", "line")
    kind = "match"
    def __init__(self, scrutinee, arms, line=0):
        # arms: list of (pattern, guard, body)
        self.scrutinee = scrutinee; self.arms = arms; self.line = line

class FuncDef(Node):
    __slots__ = ("name", "params", "body", "is_async", "is_generator", "decorators", "line")
    kind = "func"
    def __init__(self, name, params, body, is_async=False, is_generator=False, decorators=None, line=0):
        self.name = name; self.params = params; self.body = body
        self.is_async = is_async; self.is_generator = is_generator
        self.decorators = decorators or []; self.line = line

class SubDef(Node):
    __slots__ = ("name", "params", "body", "decorators", "line")
    kind = "sub"
    def __init__(self, name, params, body, decorators=None, line=0):
        self.name = name; self.params = params; self.body = body
        self.decorators = decorators or []; self.line = line

class ReturnStmt(Node):
    __slots__ = ("value", "line")
    kind = "return"
    def __init__(self, value, line=0): self.value = value; self.line = line

class BreakStmt(Node):
    __slots__ = ("line")
    kind = "break"
    def __init__(self, line=0): self.line = line

class ContinueStmt(Node):
    __slots__ = ("line")
    kind = "continue"
    def __init__(self, line=0): self.line = line

class PassStmt(Node):
    __slots__ = ("line")
    kind = "pass"
    def __init__(self, line=0): self.line = line

class StopStmt(Node):
    __slots__ = ("line")
    kind = "stop"
    def __init__(self, line=0): self.line = line

class YieldStmt(Node):
    __slots__ = ("value", "line")
    kind = "yield"
    def __init__(self, value, line=0): self.value = value; self.line = line

class ThrowStmt(Node):
    __slots__ = ("value", "line")
    kind = "throw"
    def __init__(self, value, line=0): self.value = value; self.line = line

class TryStmt(Node):
    __slots__ = ("body", "catches", "finally_body", "line")
    kind = "try"
    def __init__(self, body, catches, finally_body, line=0):
        # catches: list of (exc_name_pattern, body)
        self.body = body; self.catches = catches; self.finally_body = finally_body; self.line = line

class ClassDef(Node):
    __slots__ = ("name", "base", "members", "decorators", "line")
    kind = "class"
    def __init__(self, name, base, members, decorators=None, line=0):
        self.name = name; self.base = base; self.members = members
        self.decorators = decorators or []; self.line = line

class EnumDef(Node):
    __slots__ = ("name", "variants", "line")
    kind = "enum"
    def __init__(self, name, variants, line=0):
        self.name = name; self.variants = variants; self.line = line

class ImportStmt(Node):
    __slots__ = ("module", "names", "alias", "line")
    kind = "import"
    def __init__(self, module, names, alias, line=0):
        self.module = module; self.names = names; self.alias = alias; self.line = line

class ExportStmt(Node):
    __slots__ = ("names", "line")
    kind = "export"
    def __init__(self, names, line=0): self.names = names; self.line = line

class GotoStmt(Node):
    __slots__ = ("target", "line")
    kind = "goto"
    def __init__(self, target, line=0): self.target = target; self.line = line

class GosubStmt(Node):
    __slots__ = ("target", "line")
    kind = "gosub"
    def __init__(self, target, line=0): self.target = target; self.line = line

class DataStmt(Node):
    __slots__ = ("values", "line")
    kind = "data"
    def __init__(self, values, line=0): self.values = values; self.line = line

class ReadStmt(Node):
    __slots__ = ("vars", "line")
    kind = "read"
    def __init__(self, vars, line=0): self.vars = vars; self.line = line

class RestoreStmt(Node):
    __slots__ = ("line")
    kind = "restore"
    def __init__(self, line=0): self.line = line

class DeferStmt(Node):
    __slots__ = ("call", "line")
    kind = "defer"
    def __init__(self, call, line=0): self.call = call; self.line = line

class SpawnStmt(Node):
    __slots__ = ("call", "line")
    kind = "spawn"
    def __init__(self, call, line=0): self.call = call; self.line = line

class SendStmt(Node):
    """chan <- value  (channel send)."""
    __slots__ = ("chan", "value", "line")
    kind = "send"
    def __init__(self, chan, value, line=0):
        self.chan = chan; self.value = value; self.line = line

class WithStmt(Node):
    __slots__ = ("expr", "as_name", "body", "line")
    kind = "with"
    def __init__(self, expr, as_name, body, line=0):
        self.expr = expr; self.as_name = as_name; self.body = body; self.line = line

class OptionBaseStmt(Node):
    __slots__ = ("base", "line")
    kind = "optionbase"
    def __init__(self, base, line=0): self.base = base; self.line = line

class RandomizeStmt(Node):
    __slots__ = ("seed", "line")
    kind = "randomize"
    def __init__(self, seed, line=0): self.seed = seed; self.line = line
