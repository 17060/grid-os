# Colon commands

Press **Esc**, type the command (with leading `:`), press **Enter**.

Aliases are shown in parentheses.

---

## `:run` (`:r`)

**Where:** IDE colon bar  
**Syntax:** `:run`  
**Description:** Run the program currently in the editor buffer (same as executing `:list` contents).  
**Example:**

```text
Esc :run
```

**See also:** `:list`, `:compile`, `:tutorial`

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

**Syntax:** `:load tutorial`  
**Description:** Load `/programs/tutorial.bas` into the editor (path is under `/programs/`).  
**Example:**

```text
Esc :load hello
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

## `:mods`

**Syntax:** `:mods`  
**Description:** List installed IDE modules (`pkg mods`).  
**Example:**

```text
Esc :mods
```

**See also:** [Package modules](package-modules.md)

---

## `:mod run <name>`

**Syntax:** `:mod run grid-ping`  
**Description:** Run a package module by name (fullscreen; key to return).  
**Example:**

```text
Esc :mod run disc-status
Esc :mod run ide-cheatsheet
```

**See also:** `:mod load`, `basic mod run`

---

## `:mod load <name>`

**Syntax:** `:mod load pkg-index`  
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
| `:ai models` | Show bridge model info |
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

Requires **STORAGE** capability.

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
| `btc send <addr> <amt>` | Send coins |

**Example:**

```text
Esc btc status
Esc btc balance
```

Host: `make btc-bridge` (TCP :8767).

---

## Shell passthrough

Any command from [Shell from IDE](shell-from-ide.md) works **without** a leading colon:

```text
Esc pkg mods
Esc net ping gateway
Esc help
Esc poweroff
```

**See also:** [Shell from IDE](shell-from-ide.md)
