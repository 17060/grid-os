# AssimBASIC 7.2

**AssimBASIC** is Grid OS’s advanced BASIC — classic GridBASIC plus the best ideas
assimilated from across the programming-language universe, with first-class
**IRC**, **cryptocurrency (BTC)**, and **AI model** bindings.

Run demos:

```text
grid> basic run /programs/assimdemo.bas
grid> basic help
Esc :ai ask What is MATCH in AssimBASIC?
```

Host bridges (optional live backends):

```text
make ai-bridge    # TCP :8766 — LLM models (GRID.AI.RUN$ / CHAT$ / ASK$)
make btc-bridge   # TCP :8767 — Bitcoin Core RPC
```

## Assimilated language features

| Feature | Inspired by | Example |
|--------|-------------|---------|
| `TRY` / `CATCH` / `FINALLY` / `END TRY` | Python, Java | Structured error handling (`ERR$`) |
| `MATCH` / `WHEN` / `OTHERWISE` / `END MATCH` | Rust, Scala | Pattern-style dispatch (also `SELECT CASE`) |
| `UNLESS … THEN` | Ruby, Perl | Inverted `IF` |
| `FOREACH I IN ARR` … `NEXT` | Python, JS | Index walk over a `DIM` array |
| `LOOP` … `END LOOP` + `BREAK` | C, Go, JS | Infinite loop + exit |
| `ASSERT expr` | Many | Fail into `TRY`/`ON ERROR` if false |
| `SWAP a, b` | Python tuple swap | Exchange scalars |
| `+=` `-=` `*=` `/=` | C, JS, Python | Compound assignment |
| `IIF(c,a,b)` | VB / Excel | Ternary expression |
| `TYPEOF$` | JS `typeof` | `"number"` / `"string"` |
| `CLAMP` / `BETWEEN` | Rust / math libs | Range helpers |
| `REPLACE$` / `FIELD$` | Python / awk | String rewrite / nth field |
| `XOR` | Many | Boolean exclusive-or |

Classic GridBASIC remains fully supported (SUB/FUNCTION, `#IF`/`#INCLUDE`,
bytecode `.grid`, CONST, DATA/READ, ON ERROR, 2D DIM, …).

## IRC

```basic
10 GRID.IRC.CONNECT "gateway", 6667, "flynn"
20 GRID.IRC.JOIN "#grid"
30 GRID.IRC.SAY "#grid", "end of line"
40 GRID.IRC.PRIVMSG "#grid", "AssimBASIC online"
50 PRINT GRID.IRC.STATUS$
60 PRINT GRID.IRC.CONNECTED
70 PRINT GRID.IRC.READ$
80 GRID.IRC.DISCONNECT
```

Shell: `irc connect|join|say|read|status`.

## Cryptocurrency (Bitcoin)

```basic
10 PRINT GRID.BTC.STATUS$
20 PRINT GRID.BTC.BALANCE$
30 PRINT GRID.BTC.ADDRESS$("flynn")
40 R$ = GRID.BTC.SEND$("tb1q…", "0.001")
50 PRINT GRID.BTC.TX$(txid$)
60 PRINT GRID.BTC.BLOCK$("100")
70 GRID.BTC.PRINT "getblockchaininfo"
```

Requires `make btc-bridge` (testnet/regtest recommended).

## AI model running

```basic
10 PRINT GRID.AI.MODELS$
20 PRINT GRID.AI.RUN$("Write a FOR loop that prints 1 to 5")
30 PRINT GRID.AI.CHAT$("What is TRY/CATCH in AssimBASIC?")
40 PRINT GRID.AI.ASK$("Explain FOREACH", "EXPLAIN")
50 GRID.AI.PRINT "Complete: FOR I=1 TO 3", "COMPLETE"
```

| Binding | Role |
|---------|------|
| `GRID.AI.RUN$` | Run a prompt against the host LLM (model runner) |
| `GRID.AI.CHAT$` | Conversational reply |
| `GRID.AI.ASK$` / `EXPLAIN$` / `FIX$` / `COMPLETE$` | Task-specific helpers |
| `GRID.AI.MODELS$` | List bridge / model info |
| `GRID.AI.PRINT …, "RUN"\|"CHAT"\|…` | Full-length console output |

Offline keyword fallbacks work without a bridge; live models need `make ai-bridge`.

## Samples

| Path | Purpose |
|------|---------|
| `/programs/assimdemo.bas` | Full AssimBASIC showcase (language + AI/BTC/IRC status) |
| `/programs/hello.bas` | Minimal AssimBASIC hello |
| `/programs/aidemo.bas` | AI PRINT demo |
| `/programs/btc-demo.bas` | BTC bridge demo |

## See also

- [docs/wiki/](wiki/README.md) — IDE encyclopedia
- [docs/GETTING_STARTED.md](GETTING_STARTED.md) — boot, bridges, IDE
- [docs/NETWORKING.md](NETWORKING.md) — IRC / HTTP / DNS
