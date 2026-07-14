# GridBasic

**The most advanced BASIC on Earth** — a GUI IDE and a modern, multi-paradigm
BASIC programming language that takes the best features from across the whole
universe of programming languages and integrates them into one approachable,
line-oriented syntax. It ships with **IRC**, **cryptocurrency**, and **AI
model running** built in.

GridBasic runs anywhere Python 3.8+ runs. The whole thing — language, modules,
IDE — uses **only the Python standard library**. No `pip install` required.

```
┌──────────────────────────────────────────────────────────────────┐
│  GridBasic IDE                                                   │
│  ┌─────────┬──────────────────────────────┬───────────────────┐  │
│  │ Files   │  editor + syntax highlight    │ Console / Canvas  │  │
│  │ tour.gb │  PRINT "Hello, GridBasic!"    │ / IRC / Crypto    │  │
│  │ ...     │  for x in 1..5                │ / AI panels       │  │
│  │         │    print x^2                  │                   │  │
│  │         │  next                         │                   │  │
│  └─────────┴──────────────────────────────┴───────────────────┘  │
│  ▶ Run  ■ Stop  New  Save  Open  Examples ▾     Ln 1, Col 1      │
└──────────────────────────────────────────────────────────────────┘
```

## Quick start

```bash
cd gridbasic

# run a program
python3 -m gridbasic examples/tour.gb

# one-liner
python3 -m gridbasic -c 'print "Hello, GridBasic!"'

# launch the GUI IDE (then open http://localhost:8765)
python3 -m gridbasic --ide

# REPL
python3 -m gridbasic
```

Run the test suite:

```bash
python3 tests/test_gridbasic.py        # 44 tests, 0 deps
```

## What makes GridBasic the most advanced BASIC

GridBasic keeps the friendly, line-oriented spirit of classic BASIC
(`PRINT`, `INPUT`, `FOR/NEXT`, `GOTO/GOSUB`, line labels, `DATA/READ`,
`SELECT CASE`, `PRINT USING`) and layers on the best ideas from many languages:

| Borrowed from        | Feature in GridBasic                                            |
|----------------------|-----------------------------------------------------------------|
| **Python**           | list/dict comprehensions, f-strings, generators (`yield`), decorators, first-class functions, dynamic typing with optional hints |
| **JavaScript/TS**    | arrow functions `(x) => x*x`, `async`/`await`, destructuring, optional chaining `?.`, nullish coalescing `??`, spread `...` |
| **Rust / ML**        | `match`/`case` pattern matching with guards, tuple & list patterns |
| **Go**               | lightweight concurrency (`spawn`), channels (`chan`, `<-`), `defer` |
| **Swift / Kotlin**   | `enum`, `class`/`extends`, `NEW`, `ME`/`self`, smart flow       |
| **Lisp**             | functions-as-values, anonymous `lambda`                         |
| **SQL / Haskell**    | comprehension query syntax, ranges `1..5`, `1..=5`             |
| **Classic BASIC**    | line numbers, `GOTO`/`GOSUB`/`RETURN`, `REM`/`'`, `PRINT USING` |

### A taste of the language

```basic
' f-strings + ranges + comprehensions
print f"2+2={2+2}, squares={[x*x for x in 1..=5]}"

' functions, closures, arrow functions
function counter()
  let n = 0
  return () => { n = n + 1; return n }
end function
fn sq = (x) => x * x

' classes with inheritance and ME
CLASS Dog EXTENDS Animal
  FUNCTION speak()
    return f"{ME.name} says Woof!"
  END FUNCTION
END CLASS
print NEW Dog("Rex").speak()

' pattern matching
match point
  case 0, 0        => print "origin"
  case [x, y]      => print f"({x},{y})"
  case n if n > 10 => print "big"
  case else        => print "other"
end match

' try/catch, channels, generators, async
try
  throw "boom"
catch e
  print e
end try

let ch = chan(int, 1)
spawn fn() => { for i in 1..5: ch <- i }
print <-ch          ' receive from channel

function fib()
  let a = 0; let b = 1
  while true: yield a; let t = a + b; a = b; b = t
end function
print [fib().next() for _ in 1..=10]

print await async_work()    ' async/await
```

## The three flagship capabilities

### `IRC` — real IRC client
A socket-based IRC client (with TLS) you can drive from code:

```basic
import irc
let conn = IRC.connect("irc.libera.chat", 6697, "GridBot", true)  ' TLS
conn.join("#gridbasic")
conn.on_message(fn(ch, who, msg) => print(f"[{ch}] <{who}> {msg}"))
conn.send("#gridbasic", "hello, world")
conn.loop()
```

### `CRYPTO` — real cryptocurrency stack
Pure-Python `secp256k1` ECDSA, Base58Check addresses, signed transactions,
and a hash-linked blockchain with proof-of-work mining — no external deps:

```basic
import crypto
let alice = CRYPTO.wallet()
let bob   = CRYPTO.wallet()
let sig   = CRYPTO.sign(alice, "pay bob 5 GRID")
print CRYPTO.verify(alice.pub_compressed(), "pay bob 5 GRID", sig)   ' true

let chain = CRYPTO.blockchain(3)        ' 3 leading-zero-byte difficulty
chain.transfer(alice, bob.address, 5)
chain.mine_pending(alice)
print chain.height(); " "; chain.is_valid(); " "; chain.balance_of(bob.address)
```

### `AI` — run local models + bridge to LLMs
Built-in models (markov text, perceptron, MLP with backprop, embedder,
sentiment) plus an OpenAI-compatible HTTP bridge:

```basic
import ai
let m = AI.model("markov", 2)
m.train("GridBasic is the most advanced basic on earth. ...")
print AI.generate("GridBasic", length=20)

let net = AI.model("mlp", [2, 4, 1])     ' learn XOR
net.train([[[0,0],0],[[0,1],1],[[1,0],1],[[1,1],0]], 1500)
print net.predict([0,1])                  ' ~ 1

print AI.api("explain GridBasic in one line",
             model="gpt-4o-mini", key=env("GRIDBASIC_AI_KEY"))
```

## The GUI IDE

`python3 -m gridbasic --ide` serves a single-page IDE at http://localhost:8765:

- **Editor** with custom syntax highlighting, line/column, tab-to-indent,
  `Ctrl+Enter` to run, `Ctrl+S` to save.
- **File explorer** for the workspace, plus an **Examples** dropdown.
- **Console** with live streaming output and a stdin box for `INPUT`.
- **Canvas** panel — `GRID.plot/line/circle/rect/text/color/clear` draw live
  as your program runs.
- **IRC**, **Crypto**, and **AI** panels — one-click "connect wallet / mine /
  generate" helpers that run real GridBasic under the hood.
- **Stop** button (cooperative), run/stop, multiple concurrent runs.

Draw on the canvas from a program:

```basic
import grid
GRID.clear(); GRID.color("#4ea1ff")
for x in 0..510
  GRID.plot(x, 200 + int(80 * sin(x / 40.0)))
next
```

## Project layout

```
gridbasic/
├── gridbasic/
│   ├── __main__.py          # CLI: run / REPL / --ide
│   ├── lexer.py             # tokenizer (strings, f-strings, regex, ranges)
│   ├── tokens.py            # token + keyword definitions
│   ├── parser.py            # recursive-descent + Pratt parser
│   ├── ast_nodes.py         # AST node types
│   ├── interpreter.py       # tree-walking evaluator + runtime values
│   ├── stdlib.py            # built-in functions
│   ├── errors.py            # error + control-flow signal types
│   ├── modules/
│   │   ├── irc.py           # socket IRC client (TLS)
│   │   ├── crypto.py        # secp256k1 + ECDSA + Base58Check + PoW chain
│   │   ├── ai.py            # markov + perceptron + MLP + LLM bridge
│   │   └── grid.py          # IDE canvas bridge + HTTP
│   └── ide/
│       ├── server.py        # self-contained HTTP backend
│       └── static/          # index.html, style.css, app.js
├── examples/                # hello, tour, irc_bot, crypto_wallet, ai_demo, ...
├── tests/                   # 44 tests, no dependencies
└── README.md
```

## License

Public domain / CC0. Do whatever you like with it.
