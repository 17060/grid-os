# Grid OS 6.5 — Commands & GridBASIC Reference

Type `help` at the `grid>` prompt for the built-in summary.

## Shell commands (`grid>`)

### Identity & system

| Command | Description |
|---------|-------------|
| `help` | Show command list |
| `disc` | Display identity disc |
| `whoami` | Entity type (User / Program) |
| `caps` | Granted capabilities |
| `status` | Grid runtime status |
| `cycles` | Same as `status` (elapsed cycles) |
| `vision` | Flynn's founding principles |
| `clear` | Clear screen + banner |
| `about` | About Grid OS 6.5 |
| `poweroff` / `halt` | Exit QEMU (isa-debug-exit) |

### Programs & jobs

| Command | Description |
|---------|-------------|
| `spawn [name]` | Run ring-3 program (default: `gridsh`) |
| `spawn bg <name>` | Load program as background job |
| `spawn list` / `spawn catalog` | List spawnable programs |
| `catalog` | Spawnable ring-3 programs |
| `programs` | Spawned program history |
| `jobs` | List background sandbox jobs |
| `kill <#>` / `kill all` | Stop background job(s) |
| `fg <#>` | Run background job in foreground |
| `wait` | Wait for background jobs to finish |
| `echo <text>` | Write text to the grid |

### Files & storage

| Command | Description |
|---------|-------------|
| `ls [path]` | GridFS listing (default `/`) |
| `cat <path>` | GridFS read |
| `gfs` | Flynn archive status |
| `gfs list` | List GFS root |
| `gfs seed` | Re-seed default files on disk |
| `log` | Full audit trail |
| `log tail` | Last 10 audit entries |

### Vault (key/value persistence)

| Command | Description |
|---------|-------------|
| `vault` / `vault list` | List vault nodes |
| `vault put <key> <value>` | Store node |
| `vault get <key>` | Read node |
| `vault save` | Snapshot in memory |
| `vault sync` | Persist to arcade disk |
| `vault export` | Export state over COM1 |
| `vault import` | Import state from COM1 |

### Network & comms

| Command | Description |
|---------|-------------|
| `net` / `net status` | virtio-net + IP status |
| `net ping <ip>` | ICMP echo (e.g. `10.0.2.2`) |
| `net poll` | Drain receive queue |
| `http get <host\|ip> [port] <path>` | HTTP/1.1 GET (keep-alive pool) |
| `http post <host\|ip> [port] <path> <body>` | HTTP/1.1 POST |
| `irc connect <ip> <port> <nick>` | Connect persistent IRC session |
| `irc join <#chan>` | Join channel |
| `irc say <#chan> <msg>` | Send PRIVMSG |
| `irc read` | Print queued IRC lines |
| `irc status` / `irc quit` / `irc nick <name>` | Session control |
| `irc <ip> <port> <nick> <#ch>` | Legacy one-shot join + listen |
| `portal` | GridLink portal status |
| `portal export` | Export vault frame on COM1 |
| `portal import` | Receive vault from host |
| `portal recv` | Install `/programs/*` over GridLink |
| `serial` / `serial status` | COM1 status |
| `serial write <text>` | Transmit on COM1 |
| `serial read` | Read one line from COM1 |

### ISO research zone

| Command | Description |
|---------|-------------|
| `iso` / `iso zone` | Zone status |
| `iso list` | List ISO entities |
| `iso spawn [name]` | Seed a new ISO |
| `iso inspect <id>` | Inspect genome and disc |
| `iso evolve <id>` | Mutate genome in sandbox |
| `iso quarantine <id>` | Isolate anomaly |
| `iso release <id>` | Restore quarantined ISO |
| `iso autopilot on` / `off` | Background evolution |

### GridBASIC

| Command | Description |
|---------|-------------|
| `basic` | Open GridBASIC IDE |
| `basic ide [file]` | IDE with optional load path |
| `basic run <file>` | Run `.bas` from GFS |
| `basic help` / `basic ?` | GridBASIC version + syntax summary |
| `basictest` | Deterministic interpreter self-test |

### Grid AI

| Command | Description |
|---------|-------------|
| `ai` | Show AI command summary |
| `ai ask <prompt>` | Ask the AI (host bridge or offline help) |
| `ai explain [line]` | Explain a GridBASIC line |
| `ai fix <code>` | Suggest fixed GridBASIC source |
| `ai complete <code>` | Complete a code fragment |
| `ai models` / `ai model` | Show bridge model info |

Host bridge: run `make ai-bridge` on the host (TCP port 8766, OpenAI-compatible or Ollama).

### Grid BTC

| Command | Description |
|---------|-------------|
| `btc` | Show Bitcoin command summary |
| `btc help` | List common RPC methods (via bridge) |
| `btc status` | Bridge / node connection status |
| `btc info` / `btc blockchain` | `getblockchaininfo` |
| `btc network` | `getnetworkinfo` |
| `btc wallet` | `getwalletinfo` |
| `btc balance` | `getbalance` |
| `btc address [label]` | `getnewaddress` |
| `btc send <addr> <amount>` | `sendtoaddress` |
| `btc call <method> [params-json]` | Arbitrary JSON-RPC call |
| `btc tx <txid>` | `getrawtransaction` |
| `btc block <hash\|height>` | `getblock` (verbose JSON) |
| `btc stop` | Stop Bitcoin Core on host (**dangerous**) |

Host bridge: run `make btc-bridge` on the host (TCP port 8767). Requires Bitcoin Core JSON-RPC on the host (`BITCOIN_RPC_URL`, `BITCOIN_RPC_USER`, `BITCOIN_RPC_PASSWORD`). Use **testnet** or **regtest** for development; never expose RPC to the internet.

### UI & misc

| Command | Description |
|---------|-------------|
| `ide [file.grid]` / `workshop [file]` | Grid Workbench (GEM + AmigaDOS) |
| `theme flynn` / `theme clu` | Shell prompt color theme |
| `recognizer` | Patrol flyover easter egg |

---

## GridBASIC IDE (press `Esc`, then type)

| Command | Aliases | Description |
|---------|---------|-------------|
| `:run` | `:r` | Run buffer |
| `:save <name>` | | Write to `/programs/<name>.bas` |
| `:load <name>` | | Read from GFS |
| `:new` | | Clear buffer |
| `:list` | `:l` | Print program |
| `:help` | `:h`, `:?` | IDE help |
| `:ai ask <prompt>` | | Ask the AI |
| `:ai explain` | | Explain current editor line |
| `:ai complete` | | Suggest completion for buffer |
| `:ai fix <code>` | | Suggest fixed GridBASIC code |
| `:ai models` | `:ai model` | Bridge model info |
| `:ai help` | `:ai ?` | AI help panel |
| `:ai <prompt>` | | Shorthand — bare prompt sent to AI |
| `:vault list` | | List vault nodes (fullscreen) |
| `:vault get <key>` | | Read vault node (status bar) |
| `:vault put <key> <val>` | | Store vault node |
| `:vault sync` | | Persist vault to arcade disk |
| `:btc help` / `:btc info` / `:btc balance` | | Bitcoin RPC via host bridge |
| `btc send <addr> <amt>` | | Send coins (fullscreen shell output) |
| `:irc connect <ip> <port> <nick>` | | Connect IRC (same as `irc connect`) |
| `:irc join <#chan>` / `:irc say` / `:irc read` | | IRC session from IDE |
| `:quit` | `:q` | Shows hint — use `grid> poweroff` to exit OS |

### Editing keys

| Key | Action |
|-----|--------|
| Arrows / Home / End | Move cursor |
| Up / Down | Recall prior `grid>` commands (shared shell history) |
| Enter | Split line (editor) or submit command (`grid>`) |
| Backspace | Delete char / merge line up |
| Delete | Delete char / merge line down |
| Esc | Open colon command bar (cancel empty command with Esc again) |

---

## Grid Workbench / AmigaDOS (`ide` or `workshop`)

| Command | Aliases | Description |
|---------|---------|-------------|
| `DIR` | `LIST` | Directory listing |
| `CD <path>` | | Change directory |
| `TYPE <file>` | | Display file |
| `ED` / `EDITOR` | `[file]` | Open editor |
| `EXEC` / `RUN` | `[file]` | Load + run GridScript |
| `SAVE` | | Save current file |
| `SAVEAS` | | Save-as file picker |
| `OPEN` | | Open file picker |
| `NEW` | | New file in editor |
| `ASSIGN` | | Show volume assignments |
| `DESKTOP` | | Return to GEM desktop |
| `CLI` | | AmigaDOS CLI view |
| `HELP` | `?` | Workbench help |
| `QUIT` | `BYE`, `ENDCLI` | Exit Workbench |

On the GEM desktop, keys **1–8** launch icons.

---

## GridBASIC language

### Statement keywords

`PRINT` / `?` · `LET` · `CONST` · `IF` · `THEN` · `ELSE` · `SELECT` · `CASE` · `END SELECT` · `FOR` · `TO` · `STEP` · `NEXT` · `EXIT FOR` · `WHILE` · `WEND` · `EXIT WHILE` · `REPEAT` · `UNTIL` · `GOTO` · `GOSUB` · `RETURN` · `INPUT` · `LINE INPUT` · `DIM` · `DATA` · `READ` · `RESTORE` · `RANDOMIZE` · `REM` · `'` · `END` · `STOP`

Multiple statements on one line are separated with `:` (e.g. `X = 1: PRINT X`).

### Expression keywords

`AND` · `OR` · `NOT` · `MOD` · `DIV`

### Operators

Arithmetic: `+` `-` `*` `/` `^`  
Comparisons: `=` `<>` or `#` `<` `>` `<=` `>=`  
String concat: `+` (when either side is a string)

### Variables

- Numeric: `N`, `COUNT`, …
- String: trailing `$` → `S$`, `MSG$`
- Arrays: `A(0)`, `A(10)` — 0-based after `DIM A(10)`
- Constants: `CONST MAX=10` — read-only; assignment, `READ`, or `INPUT` to a CONST variable is an error

### Control flow

**SELECT CASE** — multi-branch dispatch on an expression:

```basic
SELECT CASE N
CASE 1
  PRINT "one"
CASE 2, 3
  PRINT "two or three"
CASE ELSE
  PRINT "other"
END SELECT
```

**EXIT FOR** / **EXIT WHILE** — leave the innermost matching loop early.

**DATA / READ / RESTORE** — static numeric/string literals collected at run start; `RESTORE` resets the read pointer to the first `DATA` line.

**RANDOMIZE [seed]** — seed the pseudo-random generator used by `RND()`.

**LINE INPUT s$** — read a full line (including spaces) into string variable `s$`.

### Built-in functions

**Math:** `ABS` `INT` `SGN` `SQR` `RND` `PI`  
**String:** `LEN` `VAL` `ASC` `CHR$` `STR$` `UPPER$` `LOWER$` `LEFT$` `RIGHT$` `MID$` `INSTR$(hay$, needle$ [, start])`

### GRID.* bindings

**Statements:**

| Binding | Action |
|---------|--------|
| `GRID.CLS` | Clear screen |
| `GRID.LOG` expr | Write to audit log |
| `GRID.SPAWN` expr | Spawn ring-3 program by name |
| `GRID.SERIAL.WRITE` expr | Write to COM1 |
| `GRID.COLOR` n | Set console color |
| `GRID.WAIT` ticks | Busy-wait (timer ticks) |
| `GRID.PRINT` expr | Print + newline |
| `GRID.LOCATE` row, col | Move console cursor |
| `GRID.VAULT.PUT` key$, value$ | Store vault node |
| `GRID.VAULT.SYNC` | Persist vault to arcade disk |
| `GRID.GFS.WRITE` path$, data$ | Write GridFS file |

**Functions:**

| Binding | Returns |
|---------|---------|
| `GRID.TIME` | Timer ticks |
| `GRID.RND(n)` | Random 0…n−1 |
| `GRID.PING(ip$)` | 1 if ping OK, else 0 |
| `GRID.SERIAL.READ$` | One line from COM1 |
| `GRID.STATUS$` | Status string |
| `GRID.CAP(n)` | 1 if capability *n* is granted, else 0 |
| `GRID.INKEY$` | Next key scancode as string (empty if none) |
| `GRID.VAULT.GET$(key$)` | Vault node value |
| `GRID.VAULT.LIST$` | Comma-separated vault keys |
| `GRID.GFS.READ$(path$)` | GridFS file contents |
| `GRID.GFS.LIST$(path$)` | GridFS directory listing |
| `GRID.HTTP.GET$(host$, port, path$)` | HTTP GET response body |
| `GRID.HTTP.POST$(host$, port, path$, body$)` | HTTP POST response body |
| `GRID.AI.ASK$(prompt$)` | AI answer |
| `GRID.AI.COMPLETE$(fragment$)` | Complete BASIC fragment |
| `GRID.AI.EXPLAIN$(line$)` | Explain a BASIC line |
| `GRID.AI.FIX$(code$)` | Suggest fixed code |
| `GRID.AI.MODELS$` | Bridge / offline model info |
| `GRID.AI.PRINT prompt$ [, "ASK"\|"EXPLAIN"\|"FIX"\|"COMPLETE"\|"MODELS"]` | Full-length AI output to console |

### GRID.IRC.* bindings

| Statement / function | Description |
|---------------------|-------------|
| `GRID.IRC.CONNECT` server$, port, nick$ | Connect to IRC server |
| `GRID.IRC.CONNECT$(server$, port, nick$)` | Returns `"ok"` or error |
| `GRID.IRC.JOIN` channel$ | JOIN channel |
| `GRID.IRC.PART` channel$ | PART channel |
| `GRID.IRC.SAY` / `GRID.IRC.MSG` target$, msg$ | PRIVMSG |
| `GRID.IRC.NICK` nick$ | Change nick |
| `GRID.IRC.QUIT` | Send QUIT and disconnect |
| `GRID.IRC.POLL` | Drain TCP / process server lines |
| `GRID.IRC.DISCONNECT` | Drop session |
| `GRID.IRC.READ$` | Next queued line (empty if none) |
| `GRID.IRC.STATUS$` | Connection summary string |

### GRID.BTC.* bindings

| Statement / function | Description |
|---------------------|-------------|
| `GRID.BTC.CALL$(method$, params$)` | Generic JSON-RPC (params$ is JSON array or empty) |
| `GRID.BTC.INFO$` / `GRID.BTC.BLOCKCHAIN$` | `getblockchaininfo` |
| `GRID.BTC.NETWORK$` | `getnetworkinfo` |
| `GRID.BTC.WALLET$` | `getwalletinfo` |
| `GRID.BTC.BALANCE$` | `getbalance` |
| `GRID.BTC.ADDRESS$` [label$] | `getnewaddress` |
| `GRID.BTC.HELP$` | Bridge help text |
| `GRID.BTC.STATUS$` | Bridge connected / error string |
| `GRID.BTC.SEND` addr$, amount | `sendtoaddress` (sets error on failure) |
| `GRID.BTC.PRINT method$ [, params$]` | Full-length BTC/bridge output to console |

Host bridge: `make btc-bridge` (TCP `10.0.2.2:8767`). Without the bridge, calls return a clear offline error.

### Example program

```basic
10 FOR I = 1 TO 5
20   PRINT "grid line "; I
30 NEXT I
40 PRINT "hello grid"
50 PRINT "ticks: "; GRID.TIME
60 END
```

Run from shell: `basic run /programs/hello.bas`
