# Shell from IDE

At the GridBASIC IDE, press **Esc** and type Flynn shell commands **without** a leading colon. The shell runs fullscreen; press any key to return to the editor.

Also available at the main `grid>` prompt when not in the IDE.

---

## System

### `help`

**Syntax:** `help`  
**Description:** Print all shell commands.  
**Example:** `Esc help`

### `disc`

**Description:** Display identity disc (ASCII art + stats).  
**Example:** `Esc disc`

### `whoami`

**Description:** Entity type (User / Program).  
**Example:** `Esc whoami`

### `caps`

**Description:** Decoded capability list.  
**Example:** `Esc caps`

### `status` / `cycles`

**Description:** Grid runtime status and elapsed cycles.  
**Example:** `Esc status`

### `vision`

**Description:** Flynn's founding principles.  
**Example:** `Esc vision`

### `clear`

**Description:** Clear screen and show banner.  
**Example:** `Esc clear`

### `about`

**Description:** Grid OS version blurb.  
**Example:** `Esc about`

### `poweroff` / `halt`

**Description:** Exit QEMU (isa-debug-exit).  
**Example:** `Esc poweroff`

### `theme flynn` / `theme clu`

**Description:** Change shell prompt color theme.  
**Example:** `Esc theme flynn`

---

## Programs & jobs

### `spawn [name]`

**Syntax:** `spawn` · `spawn gridsh` · `spawn lightcycle`  
**Description:** Run ring-3 ELF from `/programs/` in foreground sandbox.  
**Example:** `Esc spawn gridsh` then `disc` · `exit`

### `spawn bg <name>`

**Description:** Load program as background job.  
**Example:** `Esc spawn bg gridloop`

### `catalog`

**Description:** List spawnable ring-3 programs.  
**Example:** `Esc catalog`

### `programs`

**Description:** History of spawned programs.  
**Example:** `Esc programs`

### `jobs`

**Description:** List background sandbox jobs.  
**Example:** `Esc jobs`

### `kill <#>` / `kill all`

**Example:** `Esc kill 1` · `Esc kill all`

### `fg <#>`

**Description:** Run background job in foreground.  
**Example:** `Esc fg 1`

### `wait`

**Description:** Block until all background jobs finish.  
**Example:** `Esc wait`

### `echo <text>`

**Example:** `Esc echo End of line.`

---

## Files & Flynn disk

### `ls [path]`

**Example:** `Esc ls` · `Esc ls /programs`

### `cat <path>`

**Example:** `Esc cat /programs/hello.bas`

### `gfs` / `gfs list` / `gfs seed`

**Description:** GFS2FLYN archive status, root listing, re-seed defaults.  
**Example:** `Esc gfs` · `Esc gfs seed`

### `log` / `log tail`

**Description:** Full audit trail or last 10 entries.  
**Example:** `Esc log tail`

---

## Vault

### `vault list` · `vault put <k> <v>` · `vault get <k>`

### `vault save` · `vault sync` · `vault export` · `vault import`

**Example:**

```text
Esc vault put motd The Grid is open
Esc vault sync
Esc vault get motd
```

**Program equivalent:**

```basic
10 GRID.VAULT.PUT "motd", "from BASIC"
20 GRID.VAULT.SYNC
30 PRINT GRID.VAULT.GET$("motd")
40 END
```

---

## Network

### `net` / `net status`

**Example:** `Esc net status`

### `net ping <host|ip>`

**Example:** `Esc net ping gateway` · `Esc net ping 10.0.2.2`

### `net poll`

**Description:** Drain network receive queue.

### `http get <host> [port] <path>`

**Example:** `Esc http get gateway 80 /`

### `http post <host> [port] <path> <body>`

**Example:** `Esc http post gateway 80 /api ok`

---

## IRC

### `irc connect <host> <port> <nick>`

### `irc join <#chan>` · `irc part <#chan>` · `irc say <#chan> <msg>`

### `irc read` · `irc status` · `irc nick <name>` · `irc quit` · `irc disconnect`

**Example:**

```text
Esc irc connect gateway 6667 flynn
Esc irc join #grid
Esc irc say #grid hello
Esc irc read
```

---

## TCP server

### `:server new` · `:server listen <port>` · `:server status` · `:server stop [port]`

Same as shell **`server listen|status|stop|help`**. `:server new` loads a GridBASIC template with custom keywords (**TIME**, **VER**, **HELLO** *name*) plus built-in **PING/HELP/STATUS/ECHO/QUIT**.

**Example:**

```text
Esc :server new
Esc :run
Esc server status
```

---

## Portal & serial

### `portal` · `portal export` · `portal import` · `portal recv` · `portal pkg` · `portal duel`

**Example:** `Esc portal pkg` (receive GridLink package on COM1)

### `serial status` · `serial write <text>` · `serial read`

**Example:** `Esc serial status`

---

## Package manager

### `pkg list` · `pkg mods` · `pkg info <name>`

### `pkg install <manifest-path>` · `pkg remove <name>` · `pkg recv`

**Example:**

```text
Esc pkg mods
Esc pkg info flynn-ide-tools
Esc basic mod run pkg-index
```

See [Package modules](package-modules.md).

---

## GridBASIC shell

### `basic` · `basic ide [file]` · `basic run <file>`

### `basic mod run <name>` · `basic mod load <name>`

### `basic compile <in.bas> <out.grid>` · `basic samples` · `basic help`

### `tutorial` · `samples` · `basictest`

**Examples:**

```text
Esc basic run /programs/hello.bas
Esc basic mod load ide-cheatsheet
Esc tutorial
Esc basictest
```

---

## Grid AI

### `ai` · `ai ask <prompt>` · `ai explain [line]` · `ai fix <code>` · `ai complete <code>` · `ai models`

**Example:** `Esc ai ask How do I use FOR loops?`

Host: `make ai-bridge`

---

## Grid BTC

### `btc` · `btc help` · `btc status` · `btc info` · `btc balance` · `btc send <addr> <amt>` · `btc call <method> [json]`

**Example:** `Esc btc status`

Host: `make btc-bridge`

---

## ISO research zone

### `iso` · `iso list` · `iso spawn [name]` · `iso inspect <id>` · `iso evolve <id>`

### `iso quarantine <id>` · `iso release <id>` · `iso autopilot on|off`

**Example:** `Esc iso list`

---

## Recognizer & Workbench

### `recognizer` · `recognizer start` · `recognizer stop` · `recognizer status`

**Example:** `Esc recognizer start`

### `ide [file.grid]` / `workshop [file]`

**Description:** Open Grid Workbench (GEM + AmigaDOS), not GridBASIC IDE.

**Example:** `Esc ide`

---

## See also

- [Colon commands](colon-commands.md) — IDE-specific `:` commands
- [COMMANDS.md](../COMMANDS.md) — flat reference table
