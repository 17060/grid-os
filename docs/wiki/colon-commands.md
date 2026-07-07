# Colon commands

Press **Esc**, type the command (with leading `:`), press **Enter**.

Aliases are shown in parentheses. For shell commands without `:`, see [Shell from IDE](shell-from-ide.md).

**Quick index:** `:run` · `:save` · `:load` · `:new` · `:list` · `:find` · `:goto` · `:compile` · `:samples` · `:tutorial` · `:mods` · `:mod run|load` · `:pkg` · `:server` · `:ircserver` · `:ai` · `:help` · `:quit` (hint only)

---

## `:run` (`:r`)

**Where:** IDE colon bar  
**Syntax:** `:run` · `:run [path]`  
**Description:** Run the editor buffer. If the buffer path ends in `.grid`, run GRIDBC bytecode. With **`path`**, load and run `/programs/<path>` (`.bas` or `.grid`) or an absolute Flynn path.  
**Example:**

```text
Esc :run
Esc :run hello
Esc :run demo.grid
```

**See also:** `:list`, `:compile`, `:tutorial`

---

## `:find <text>`

**Syntax:** `:find PRINT`  
**Description:** Case-insensitive search from the current cursor line downward. Status shows `found L<n>` or `find: no match`.  
**Example:**

```text
Esc :find GRID.
```

**See also:** `:goto`, `:list`

---

## `:goto <line>`

**Syntax:** `:goto 10`  
**Description:** Jump cursor to a **1-based** line number. Errors: `usage: goto <line>`, `goto: line out of range`.  
**Example:**

```text
Esc :goto 1
```

**See also:** `:find`

---

## `:save <name>`

**Syntax:** `:save hello`  
**Description:** Write buffer to `/programs/hello.bas` on Flynn disk.  
**Example:**

```text
Esc :save mydemo
```

**See also:** `:load`, `:new`

---

## `:load <name>`

**Syntax:** `:load tutorial` · `:load hello.grid`  
**Description:** Load into the editor from `/programs/<name>.bas`, `.grid`, or an absolute Flynn path. Shows `IDE: load failed — file not found` on failure.  
**Example:**

```text
Esc :load hello
Esc :load /programs/tutorial.bas
```

**See also:** `:save`, `:samples`

---

## `:new`

**Syntax:** `:new`  
**Description:** Clear editor buffer and reset path/dirty flag.  
**Example:**

```text
Esc :new
```

---

## `:list` (`:l`)

**Syntax:** `:list`  
**Description:** Full-screen listing of buffer; press any key to return.  
**Example:**

```text
Esc :list
```

---

## `:compile <name>`

**Syntax:** `:compile myprog`  
**Description:** Compile buffer to GRIDBC bytecode at `/programs/myprog.grid`.  
**Example:**

```basic
10 PRINT "bytecode ready"
20 END
```

```text
Esc :compile myprog
grid> basic run /programs/myprog.grid
```

**See also:** `basic compile`

---

## `:samples`

**Syntax:** `:samples`  
**Description:** Runs shell `samples` — lists `.bas` files on Flynn disk.  
**Example:**

```text
Esc :samples
```

---

## `:tutorial` (`:t`)

**Syntax:** `:tutorial`  
**Description:** Interactive multi-step GridBASIC walkthrough in the IDE.  
**Example:**

```text
Esc :tutorial
```

**See also:** `grid> tutorial`

---

## `:mods` · `:mods <category>`

**Syntax:** `:mods` · `:mods network`  
**Description:** List installed IDE modules (`pkg mods` or `pkg mods <category>`). **Requires leading `:`** — bare `mods` at `grid>` is not valid.  
**Example:**

```text
Esc :mods
Esc :mods network
Esc :mods bridge
```

**See also:** [Package modules](package-modules.md)

---

## `:mod run <name>`

**Syntax:** `:mod run grid-ping` · bare `:mod run` → usage hint  
**Description:** Run a package module by name (fullscreen; key to return). Distinguishes **module not found** vs **read failed**.  
**Example:**

```text
Esc :mod run disc-status
Esc :mod run ide-cheatsheet
```

**See also:** `:mod load`, `basic mod run`

---

## `:mod load <name>`

**Syntax:** `:mod load pkg-index` · bare `:mod load` → usage hint  
**Description:** Load module `.bas` source into the editor for study or editing.  
**Example:**

```text
Esc :mod load plot-grid
Esc :run
```

---

## `:pkg list|mods|run|load|info`

| Syntax | Description |
|--------|-------------|
| `:pkg list` | List installed packages |
| `:pkg mods` | List all IDE modules |
| `:pkg mods <category>` | Filter by category (network, disc, grid, …) |
| `:pkg run <name>` | Run module (same as `:mod run`) |
| `:pkg load <name>` | Load module into editor |
| `:pkg info <name>` | Package file/manifest details |
| `:pkg` (bare) | Same as `:pkg list` |
| `:pkg info` (no name) | `usage: pkg info <name>` |
| unknown `:pkg …` | Hint: `:pkg list\|mods\|run\|load\|info` |

**Example:**

```text
Esc :pkg mods network
Esc :pkg info flynn-ide-tools
Esc :mod run pkg-index
```

**See also:** [Package modules](package-modules.md)

---

## `:help` (`:h`, `:?`)

**Syntax:** `:help`  
**Description:** Full IDE help overlay (editing, colon commands, language summary).  
**Example:**

```text
Esc :help
```

---

## `:quit` (`:q`)

**Syntax:** `:quit`  
**Description:** Shows hint to use `poweroff` — does **not** exit the OS.  
**Example:**

```text
Esc :quit    → "use grid> poweroff to exit"
Esc poweroff
```

---

## AI commands (`:ai …`)

| Syntax | Description |
|--------|-------------|
| `:ai ask <prompt>` | Ask Grid AI (host bridge or offline help) |
| `:ai explain` | Explain current editor line |
| `:ai complete` | Suggest completion for entire buffer |
| `:ai fix <code>` | Suggest fixed GridBASIC |
| `:ai models` / `:ai model` | Show bridge model info |
| `:ai help` | AI command panel |
| `:ai <prompt>` | Shorthand for `:ai ask` |

**Example:**

```text
Esc :ai explain
Esc :ai ask What does DIM do?
Esc :ai complete
```

Host: `make ai-bridge` (TCP :8766).

**See also:** [GRID bindings](grid-bindings.md) → `GRID.AI.*`

---

## Vault (`:vault …` or `vault …`)

Requires **STORAGE** capability. From the IDE, **`vault list|get|put|sync`** only — use the main shell for `vault save`, `export`, and `import`.

| Syntax | Description |
|--------|-------------|
| `vault list` | List vault nodes (fullscreen via shell) |
| `vault get <key>` | Read node (status bar) |
| `vault put <key> <val>` | Store node |
| `vault sync` | Persist vault to arcade disk |

**Example:**

```text
Esc vault put motd hello grid
Esc vault sync
Esc vault get motd
```

**See also:** `GRID.VAULT.*` in [grid-bindings.md](grid-bindings.md)

---

## IRC (`:irc …` or `irc …`)

| Syntax | Description |
|--------|-------------|
| `irc connect <host> <port> <nick>` | Connect session |
| `irc join <#chan>` | Join channel |
| `irc part <#chan>` | Leave channel |
| `irc say <#chan> <msg>` | Send PRIVMSG |
| `irc read` | Print queued lines |
| `irc status` | Session summary |
| `irc nick <name>` | Change nick |
| `irc quit` / `irc disconnect` | End session |

**Example:**

```text
Esc irc connect gateway 6667 griduser
Esc irc join #grid
Esc irc say #grid End of line.
```

---

## TCP server (`:server …` or `server …`)

| Syntax | Description |
|--------|-------------|
| `:server new` | Load editable TCP server template into the buffer |
| `:server listen <port>` | Open listen port (same as shell `server listen`) |
| `:server status` | Show listeners and connected clients |
| `:server stop [port]` | Unlisten one port or stop all |
| `:server help` | Command summary |

After `:server new`, edit custom keywords in the template (e.g. **TIME**, **VER**, **HELLO** *name*), then `:run`.

Built-in client commands handled by `GRID.SERVER.BUILTIN`: **PING**, **HELP**, **STATUS**, **ECHO** *text*, **QUIT**.

**Example:**

```text
Esc :server new
Esc :run
```

From another host (QEMU user net): `nc -v 127.0.0.1 7700` then type `PING` or `HELLO Flynn`.

**See also:** [grid-bindings.md](grid-bindings.md) — `GRID.SERVER.*`

---

## IRC server (`:ircserver …` or `ircserver …`)

| Syntax | Description |
|--------|-------------|
| `:ircserver new` | Load editable IRC bot template (`!time`, `!help`, `!motd`, `!ver`) |
| `:ircserver listen <port>` | Open IRC listen port (6667 typical) |
| `:ircserver status` | Show listeners and connected clients |
| `:ircserver stop [port]` | Unlisten or stop all |
| `:ircserver help` | Command summary |

Real IRC clients can connect. Channel **`!commands`** are handled by your GridBASIC bot; normal chat is relayed.

**Example:**

```text
Esc :ircserver new
Esc :run
```

In another session: `irc connect localhost 6667 flynn`, `irc join #grid`, then type `!help`.

**See also:** [grid-bindings.md](grid-bindings.md) — `GRID.IRCSERVER.*`

---

## Bitcoin (`btc …`)

| Syntax | Description |
|--------|-------------|
| `btc` | Command summary |
| `btc help` / `btc info` / `btc balance` | RPC via host bridge |
| `btc blockchain` / `btc network` / `btc wallet` | Chain / network / wallet info |
| `btc address [label]` | New receive address |
| `btc tx <txid>` / `btc block <hash>` | Lookup transaction or block |
| `btc send <addr> <amt>` | Send coins |
| `btc call <method> [json]` | Generic JSON-RPC |
| `btc stop` | Stop bridge session |

**Example:**

```text
Esc btc status
Esc btc balance
```

Host: `make btc-bridge` (TCP :8767).

---

## Shell passthrough

Most commands from [Shell from IDE](shell-from-ide.md) work **without** a leading colon. IDE-only package listing uses **`:mods`** (not bare `mods`).

```text
Esc pkg mods
Esc net ping gateway
Esc :mods network
Esc help
Esc poweroff
```

**See also:** [Shell from IDE](shell-from-ide.md)
