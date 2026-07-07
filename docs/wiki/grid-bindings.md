# GRID bindings

**`GRID.*`** names call Flynn kernel services from GridBASIC. Use as statements (side effect) or functions (return value). String functions end with **`$`**.

---

## Console & time

### `GRID.CLS` (stmt)

Clear the text screen.  
**Example:** `10 GRID.CLS`

### `GRID.COLOR` n (stmt)

Set console color attribute (0–15 range).  
**Example:** `10 GRID.COLOR 2`

### `GRID.PRINT` expr (stmt)

Print expression and newline (bypasses line length limits vs string concat).  
**Example:** `10 GRID.PRINT GRID.STATUS$`

### `GRID.LOCATE` row, col (stmt)

Move console cursor.  
**Example:** `10 GRID.LOCATE 5, 10`

### `GRID.WAIT` ticks (stmt)

Busy-wait for timer ticks.  
**Example:** `10 GRID.WAIT 30`

### `GRID.LOG` msg (stmt)

Append to audit log.  
**Example:** `10 GRID.LOG "user ran demo"`

### `GRID.TIME` (fn)

Elapsed timer ticks.  
**Example:** `10 PRINT GRID.TIME`

### `GRID.RND(n)` (fn)

Random integer 0 … n−1.  
**Example:** `10 PRINT GRID.RND(100)`

### `GRID.INKEY$` (fn)

Next key scancode as string, or empty.  
**Example:** `10 K$ = GRID.INKEY$`

### `GRID.STATUS$` (fn)

OS status string.  
**Example:** `10 PRINT GRID.STATUS$`

---

## Identity & security

| Binding | Type | Example |
|---------|------|---------|
| `GRID.WHOAMI$` | fn | `PRINT GRID.WHOAMI$` |
| `GRID.CAPS$` | fn | Capability bitmask (decimal string) |
| `GRID.CAP(n)` | fn | Test capability bit *n* |
| `GRID.CAP()` | fn | Shorthand — true if `CAP_READ_GRID` granted |
| `GRID.DISC.STATUS$` | fn | Disc summary line |
| `GRID.DISC.ENTITY$` | fn | User / Program |
| `GRID.DISC.LEVEL` | fn | Disc level (numeric) |
| `GRID.DISC.XP` | fn | Disc XP (numeric) |

**Sample:**

```basic
10 GRID.CLS
20 PRINT GRID.DISC.STATUS$
30 PRINT "Level "; GRID.DISC.LEVEL
40 END
```

---

## Network

| Binding | Example |
|---------|---------|
| `GRID.PING(host$)` | `PRINT GRID.PING("gateway")` → 1 if OK |
| `GRID.DNS.RESOLVE$(host$)` | `PRINT GRID.DNS.RESOLVE$("grid")` |
| `GRID.NET.STATUS$` | `PRINT GRID.NET.STATUS$` |
| `GRID.HTTP.GET$(host$, port, path$)` | `R$ = GRID.HTTP.GET$("gateway", 80, "/")` |
| `GRID.HTTP.POST$(host$, port, path$, body$)` | POST body |

**Sample:**

```basic
10 PRINT "ping: "; GRID.PING("gateway")
20 PRINT GRID.DNS.RESOLVE$("bridge")
30 END
```

---

## Vault & Flynn disk (GFS)

| Binding | Type | Example |
|---------|------|---------|
| `GRID.VAULT.GET$(key$)` | fn | Read vault node |
| `GRID.VAULT.LIST$` | fn | Comma-separated keys |
| `GRID.VAULT.PUT` key$, val$ | stmt | Store node |
| `GRID.VAULT.SYNC` | stmt | Persist to arcade disk |
| `GRID.VAULT.EXPORT` | stmt | Export over COM1 |
| `GRID.VAULT.IMPORT` | stmt | Import from COM1 |
| `GRID.GFS.READ$(path$)` | fn | Read file contents |
| `GRID.GFS.LIST$(prefix$)` | fn | `GRID.GFS.LIST$("/programs")` |
| `GRID.GFS.WRITE` path$, data$ | stmt | Write file |

**Sample:**

```basic
10 GRID.VAULT.PUT "motd", "hello from BASIC"
20 GRID.VAULT.SYNC
30 PRINT GRID.VAULT.GET$("motd")
40 END
```

---

## Jobs & ISO

| Binding | Example |
|---------|---------|
| `GRID.SPAWN` name$ | `GRID.SPAWN "gridsh"` |
| `GRID.SPAWN.BG` name$ | Background sandbox job |
| `GRID.JOBS.LIST$` | `PRINT GRID.JOBS.LIST$` |
| `GRID.JOBS.KILL` n | Stop job number n |
| `GRID.ISO.LIST$` | ISO entity list |
| `GRID.ISO.SPAWN` name$ | Seed ISO entity |
| `GRID.ISO.EVOLVE` id | Mutate ISO genome |
| `GRID.WORKSHOP.SPAWN` name$ | Grid Workbench spawn |

---

## Graphics & sound

| Binding | Example |
|---------|---------|
| `GRID.PLOT` x, y, c | `GRID.PLOT 10, 10, 2` |
| `GRID.LINE` x0,y0,x1,y1,c | Draw line |
| `GRID.CIRCLE` cx,cy,r,c | Draw circle |
| `GRID.BEEP` freq, ms | `GRID.BEEP 880, 200` |
| `GRID.NOTE` note, ms | `GRID.NOTE 60, 120` |

**Sample:**

```basic
10 GRID.CLS
20 FOR I = 0 TO 40
30   GRID.PLOT I, I, 3
40 NEXT I
50 END
```

---

## Recognizer patrol

| Binding | Example |
|---------|---------|
| `GRID.RECOGNIZER.START` | Start background patrol |
| `GRID.RECOGNIZER.STOP` | Stop patrol |
| `GRID.RECOGNIZER.STATUS$` | `PRINT GRID.RECOGNIZER.STATUS$` |

---

## Portal & serial

| Binding | Description |
|---------|-------------|
| `GRID.PORTAL.RECV` | Receive GridLink file |
| `GRID.PORTAL.PKG` | Receive GridLink package |
| `GRID.PORTAL.DUEL` | Lightcycle duel ping |
| `GRID.SERIAL.READ$` | Read COM1 line |
| `GRID.SERIAL.WRITE` expr | Write COM1 |

---

## Packages

| Binding | Example |
|---------|---------|
| `GRID.PKG.LIST$` | Installed package names |
| `GRID.PKG.MODS$` | IDE module names (comma-separated) |
| `GRID.PKG.MOD.LIST$` | Alias for `GRID.PKG.MODS$` |
| `GRID.PKG.INSTALL` path$ | Register MANIFEST (**STORAGE** cap) |
| `GRID.PKG.REMOVE` name$ | Uninstall package (**STORAGE** cap) |
| `GRID.PKG.MOD.RUN` name$ | Run IDE module |
| `GRID.PKG.RECV` | Receive PKG on COM1 (**STORAGE** cap) |

**Sample:**

```basic
10 PRINT GRID.PKG.MODS$
20 GRID.PKG.MOD.RUN "grid-ping"
30 END
```

---

## Audit

| Binding | Example |
|---------|---------|
| `GRID.LOG.TAIL$(n)` | Last n log lines |

```basic
10 PRINT GRID.LOG.TAIL$(5)
20 END
```

---

## Grid AI

| Binding | Description |
|---------|-------------|
| `GRID.AI.ASK$(prompt$)` | Ask AI |
| `GRID.AI.COMPLETE$(fragment$)` | Complete code |
| `GRID.AI.EXPLAIN$(line$)` | Explain line |
| `GRID.AI.FIX$(code$)` | Suggest fix |
| `GRID.AI.MODELS$` | Bridge info |
| `GRID.AI.PRINT` prompt$ [, mode$] | Full console output (ASK/EXPLAIN/FIX/COMPLETE/MODELS) |

Host: `make ai-bridge` (TCP :8766)

**Sample:**

```basic
10 PRINT GRID.AI.EXPLAIN$("What does PRINT do?")
20 GRID.AI.PRINT "Explain DIM", "EXPLAIN"
30 END
```

---

## Grid IRC

| Statement | Args |
|-----------|------|
| `GRID.IRC.CONNECT` | host$, port, nick$ |
| `GRID.IRC.CONNECT$(…)` | Returns `"ok"` or error |
| `GRID.IRC.JOIN` | channel$ |
| `GRID.IRC.PART` | channel$ |
| `GRID.IRC.SAY` / `GRID.IRC.MSG` | target$, message$ |
| `GRID.IRC.NICK` | nick$ |
| `GRID.IRC.QUIT` | — |
| `GRID.IRC.POLL` | Process incoming |
| `GRID.IRC.DISCONNECT` | Drop session |

| Function | Returns |
|----------|---------|
| `GRID.IRC.READ$` | Next line or empty |
| `GRID.IRC.STATUS$` | Session summary |

---

## Grid TCP server

Line-oriented TCP server for custom command protocols from GridBASIC.

| Binding | Role |
|---------|------|
| `GRID.SERVER.LISTEN` port | Start listening (stmt) |
| `GRID.SERVER.LISTEN$(port)` | Returns `"ok"` or error |
| `GRID.SERVER.POLL` | Poll network + read client lines |
| `GRID.SERVER.ACCEPT$()` | Accept client; returns slot number string |
| `GRID.SERVER.CMD$(slot)` | Next complete line from client |
| `GRID.SERVER.WRITE` slot, text | Raw write (no CRLF) |
| `GRID.SERVER.REPLY` slot, text | Write line with CRLF |
| `GRID.SERVER.BUILTIN` slot, line | Dispatch PING/HELP/STATUS/ECHO/QUIT |
| `GRID.SERVER.BUILTIN$(slot, line$)` | `"1"` if handled, else `"0"` |
| `GRID.SERVER.CLOSE` slot | Close client |
| `GRID.SERVER.STOP` [port] | Unlisten port or stop all |
| `GRID.SERVER.STATUS$` | Listener + client summary |

Built-in commands: **PING**, **HELP**, **STATUS**, **ECHO** *text*, **QUIT** / **EXIT**.

**IDE:** `Esc` `:server new` loads an editable template with sample custom keywords (**TIME**, **VER**, **HELLO** *name*).

**Shell:** `server listen 7700`, `server status`, `server stop`

**Sample:**

```basic
10 R$ = GRID.SERVER.LISTEN$(7700)
20 WHILE 1
30   GRID.SERVER.POLL
40   S = VAL(GRID.SERVER.ACCEPT$())
50   IF S > 0 THEN GOSUB SERVE
60   FOR SI = 1 TO 8
70     S = SI
80     CMD$ = GRID.SERVER.CMD$(S)
90     IF CMD$ <> "" THEN GOSUB SERVE
100  NEXT SI
110 WEND
120 SERVE:
130  IF CMD$ = "TIME" THEN GRID.SERVER.REPLY S, STR$(GRID.TIME): RETURN
140  IF GRID.SERVER.BUILTIN$(S, CMD$) = "0" THEN GRID.SERVER.REPLY S, "502 unknown"
150  RETURN
```

---

## Flynn IRC server

Real IRC protocol server for GridBASIC bots with **`!` command** hooks.

| Binding | Role |
|---------|------|
| `GRID.IRCSERVER.LISTEN` port | Start IRC listen (stmt) |
| `GRID.IRCSERVER.LISTEN$(port)` | Returns `"ok"` or error |
| `GRID.IRCSERVER.POLL` | Accept clients + process IRC wire protocol |
| `GRID.IRCSERVER.EVENT$()` | Next bot event (`PRIVMSG\|slot\|nick\|target\|text`) |
| `GRID.IRCSERVER.KIND$` | Last event kind (`PRIVMSG`, `JOIN`, `PART`, `QUIT`) |
| `GRID.IRCSERVER.ENICK$` / `ETARGET$` / `ETEXT$` | Last event fields |
| `GRID.IRCSERVER.ESLOT` | Last event client slot |
| `GRID.IRCSERVER.NICK$(slot)` | Connected client nick |
| `GRID.IRCSERVER.SAY` slot, target$, text$ | PRIVMSG as client |
| `GRID.IRCSERVER.NOTICE` slot, target$, text$ | NOTICE as client |
| `GRID.IRCSERVER.BOT.SAY` target$, text$ | Bot reply as `grid.flynn` |
| `GRID.IRCSERVER.BOT.NOTICE` target$, text$ | Bot notice |
| `GRID.IRCSERVER.STOP` [port] | Stop IRC server |
| `GRID.IRCSERVER.STATUS$` | Listener + client summary |

Channel messages starting with **`!`** (e.g. `!time`) are **not broadcast** — they queue a `PRIVMSG` event for your GridBASIC bot loop. Normal chat is relayed to the channel.

**IDE:** `Esc` `:ircserver new` — editable bot with **!time**, **!help**, **!motd**, **!ver**

**Shell:** `ircserver listen 6667`

**Sample:**

```basic
10 R$ = GRID.IRCSERVER.LISTEN$(6667)
20 WHILE 1
30   GRID.IRCSERVER.POLL
40   E$ = GRID.IRCSERVER.EVENT$()
50   IF E$ <> "" AND GRID.IRCSERVER.KIND$ = "PRIVMSG" THEN
60     IF GRID.IRCSERVER.ETEXT$ = "!time" THEN GRID.IRCSERVER.BOT.SAY "#grid", STR$(GRID.TIME)
70 WEND
```

Connect from Grid OS shell: `irc connect localhost 6667 flynn` then `irc join #grid`.

---

## Grid BTC

| Binding | RPC / role |
|---------|------------|
| `GRID.BTC.CALL$(method$, params$)` | Generic JSON-RPC |
| `GRID.BTC.INFO$` / `GRID.BTC.BLOCKCHAIN$` | getblockchaininfo |
| `GRID.BTC.NETWORK$` | getnetworkinfo |
| `GRID.BTC.WALLET$` | getwalletinfo |
| `GRID.BTC.BALANCE$` | getbalance |
| `GRID.BTC.ADDRESS$` [label$] | getnewaddress |
| `GRID.BTC.HELP$` | Help text |
| `GRID.BTC.STATUS$` | Bridge status |
| `GRID.BTC.SEND` addr$, amount | sendtoaddress |
| `GRID.BTC.PRINT` method$ [, params$] | Full RPC output |

Host: `make btc-bridge` (TCP :8767)

---

## See also

- [Package modules](package-modules.md)
- [Shell from IDE](shell-from-ide.md) — `ai`, `btc`, `irc`, `pkg`
- [Colon commands](colon-commands.md) — `:ai`, `vault`, `irc`, `btc`
