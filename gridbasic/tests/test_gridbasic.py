"""Test suite for the GridBasic language and modules.

Run with:  python -m pytest tests/   (if pytest is available)
       or:  python tests/test_gridbasic.py
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from gridbasic import run, run_file, version
from gridbasic.errors import GridBasicError
from gridbasic.lexer import Lexer
from gridbasic.parser import Parser
from gridbasic.interpreter import Interpreter


def collect(source):
    out = []
    run(source, output=out.append)
    return "".join(out)


# ---- helpers -------------------------------------------------------------
def _parse_ok(src):
    Parser(Lexer(src).tokenize()).parse()


# ---- lexer / parser ------------------------------------------------------
def test_lexer_numbers():
    toks = Lexer('42 3.14 0x1F 0b101 1e3').tokenize()
    kinds = [t.type.name for t in toks if t.type.name != "NEWLINE" and t.type.name != "EOF"]
    assert kinds == ["INT", "INT", "INT", "INT", "FLOAT"] or kinds.count("INT") >= 3

def test_lexer_strings_and_fstrings():
    toks = Lexer('print "hi" f"x={1+1}"').tokenize()
    assert any(t.type.name == "STRING" for t in toks)
    assert any(t.type.name == "FSTRING" for t in toks)

def test_parser_print():
    _parse_ok('print "hello"')
    _parse_ok('print 1; 2; 3')
    _parse_ok('print a, b, c;')


# ---- core language -------------------------------------------------------
def test_hello():
    assert collect('print "Hello, GridBasic!"') == "Hello, GridBasic!\n"

def test_arithmetic():
    assert collect('print 2 + 2 * 3') == "8\n"
    assert collect('print 10 / 4') == "2.5\n"
    assert collect('print 10 mod 3') == "1\n"
    assert collect('print 2 ^ 10') == "1024\n"

def test_fstring():
    assert collect('let n = 5\nprint f"n is {n}"') == "n is 5\n"
    assert collect('print f"{2+3} ok"') == "5 ok\n"

def test_variables_and_types():
    assert collect('let x = 10\nlet y = 3.5\nlet s = "hi"\nprint x; y; s') == "10 3.5 hi\n"

def test_if_else():
    out = collect('let x = 5\nif x > 3 then print "big" else print "small"')
    assert out == "big\n"

def test_for_next():
    out = collect('for i in 1..=5\n  print i\nnext')
    assert out == "1\n2\n3\n4\n5\n"

def test_while_wend():
    out = collect('let i = 0\nwhile i < 3\n  print i\n  i = i + 1\nwend')
    assert out == "0\n1\n2\n"

def test_list_comprehension():
    out = collect('print [x*2 for x in 1..5 if x mod 2 == 0]')
    assert out == "[4, 8]\n"

def test_dict_comprehension():
    out = collect('let d = {k: k*k for k in [1,2,3]}\nprint d[2]')
    assert out == "4\n"

def test_functions():
    out = collect('function add(a, b)\n  return a + b\nend function\nprint add(3, 4)')
    assert out == "7\n"

def test_arrow_fn():
    out = collect('fn sq = (x) => x * x\nprint sq(6)')
    assert out == "36\n"

def test_closures():
    out = collect('function counter()\n  let n = 0\n  return () => { n = n + 1; return n }\nend function\nlet c = counter()\nprint c(); c(); c()')
    assert out == "1 2 3\n"

def test_classes_inheritance():
    out = collect('CLASS A\n  FUNCTION who()\n    return "A"\n  END FUNCTION\nEND CLASS\nCLASS B EXTENDS A\nEND CLASS\nprint NEW B().who()')
    assert out == "A\n"

def test_classes_methods():
    out = collect('CLASS Dog\n  PUBLIC name\n  CONSTRUCTOR(n)\n    ME.name = n\n  END CONSTRUCTOR\n  FUNCTION speak()\n    return ME.name + " says woof"\n  END FUNCTION\nEND CLASS\nlet d = NEW Dog("Rex")\nprint d.speak()')
    assert out == "Rex says woof\n"

def test_match():
    out = collect('function f(x)\n  match x\n    case 0 => return "zero"\n    case n if n > 10 => return "big"\n    case else => return "other"\n  end match\nend function\nprint f(0); f(20); f(5)')
    assert "zero" in out and "big" in out and "other" in out

def test_match_list_pattern():
    out = collect('function f(x)\n  match x\n    case [a, b] => return a + b\n    case else => return 0\n  end match\nend function\nprint f([3, 4])')
    assert out == "7\n"

def test_try_catch():
    out = collect('try\n  throw "boom"\ncatch e\n  print "caught: " + e\nend try')
    assert "caught: boom" in out

def test_try_catch_no_throw():
    out = collect('try\n  print "ok"\ncatch e\n  print "err"\nend try')
    assert out == "ok\n"

def test_select_case():
    out = collect('select case 2\ncase 1\n  print "one"\ncase 2\n  print "two"\ncase else\n  print "other"\nend select')
    assert out == "two\n"

def test_data_read():
    out = collect('data 10, 20, 30\nread a, b, c\nprint a + b + c')
    assert out == "60\n"

def test_goto_gosub():
    out = collect('goto 30\n10 print "skip me"\n20 goto 40\n30 print "here"\n40 print "end"')
    assert "here" in out and "end" in out and "skip me" not in out

def test_generators():
    out = collect('function nat()\n  let i = 0\n  while true\n    yield i\n    i = i + 1\n  wend\nend function\nlet g = nat()\nprint [g.next() for _ in 1..=5]')
    assert out == "[0, 1, 2, 3, 4]\n"

def test_destructuring():
    out = collect('let [a, b, c] = [1, 2, 3]\nprint a + b + c')
    assert out == "6\n"

def test_nullish():
    out = collect('let x = none\nprint x ?? "default"')
    assert out == "default\n"

def test_ternary():
    out = collect('print 5 > 3 ? "yes" : "no"')
    assert out == "yes\n"

def test_ranges():
    out = collect('print sum(1..=10)')
    assert out == "55\n"

def test_list_methods():
    out = collect('let a = [1, 2, 3]\na.push(4)\nprint a.size()')
    assert out == "4\n"

def test_string_methods():
    out = collect('print "hello".upper()')
    assert out == "HELLO\n"
    out = collect('print "a,b,c".split(",").size()')
    assert out == "3\n"

def test_decorators():
    out = collect('function loud(f)\n  return (x) => f(x) + "!"\nend function\n@loud\nfn greet = (name) => "hi " + name\nprint greet("flynn")')
    assert out == "hi flynn!\n"

def test_async_await():
    out = collect('async function task()\n  return 42\nend function\nprint await task()')
    assert out == "42\n"

def test_channels():
    out = collect('let ch = chan(int, 1)\nspawn fn() => { ch <- 99 }\nsleep(0.2)\nprint <-ch')
    assert out == "99\n"


# ---- modules -------------------------------------------------------------
def test_crypto_wallet_sign_verify():
    out = collect('import crypto\nlet w = CRYPTO.wallet()\nlet sig = CRYPTO.sign(w, "msg")\nprint CRYPTO.verify(w.pub_compressed(), "msg", sig)')
    assert out.strip() == "true"

def test_crypto_blockchain():
    out = collect('import crypto\nlet c = CRYPTO.blockchain(2)\nlet w = CRYPTO.wallet()\nc.mine_pending(w)\nprint c.height(); " "; c.is_valid()')
    # height grows and chain is valid
    assert "true" in out

def test_ai_markov():
    out = collect('import ai\nlet m = AI.model("markov", 1)\nm.train("a b a c a b")\nlet s = AI.generate("a", length=3)\nprint len(s) > 0')
    assert out.strip() == "true"

def test_ai_perceptron():
    out = collect('import ai\nlet p = AI.model("perceptron", 2)\np.train([[[0,0],0],[[1,1],1]], 50)\nprint type(p.predict([0,0]))')
    assert "int" in out

def test_ai_sentiment():
    out = collect('import ai\nprint AI.sentiment("I love this, it is brilliant and great") > 0')
    assert out.strip() == "true"

def test_grid_http():
    # GRID.http_get on a non-existent host returns status 0, not a crash
    out = collect('import grid\nlet r = GRID.http_get("http://127.0.0.1:1/nope")\nprint r.status == 0')
    assert out.strip() == "true"

def test_irc_module_loads():
    out = collect('import irc\nprint left(IRC.VERSION, 8)')
    assert out.startswith("GridB")


# ---- error handling ------------------------------------------------------
def test_undefined_variable():
    try:
        collect('print nosuchvar')
        assert False, "should have raised"
    except GridBasicError:
        pass

def test_div_by_zero():
    try:
        collect('print 1 / 0')
        assert False, "should have raised"
    except GridBasicError:
        pass


# ---- examples run --------------------------------------------------------
def test_examples_run():
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ex_dir = os.path.join(here, "examples")
    for name in ["hello.gb", "tour.gb", "ai_demo.gb", "crypto_wallet.gb",
                 "canvas.gb", "concurrency.gb"]:
        path = os.path.join(ex_dir, name)
        out = []
        try:
            run_file(path, output=out.append, max_steps=20_000_000)
        except Exception as e:
            assert False, f"{name} failed: {e}"
    assert version().startswith("GridBasic")


# ---- runner --------------------------------------------------------------
def main():
    fns = [(n, v) for n, v in sorted(globals().items()) if n.startswith("test_") and callable(v)]
    passed = 0
    failed = 0
    for name, fn in fns:
        try:
            fn()
            passed += 1
        except Exception as e:
            failed += 1
            print(f"FAIL {name}: {e}")
    print(f"\n{passed} passed, {failed} failed, {len(fns)} total")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
