# Getting Started with Grid OS 7.1.1

Flynn's Grid — a bootable x86_64 hobby OS with GridBASIC IDE, ring-3 sandboxes, multi-session TCP networking, and host bridges for AI and Bitcoin.

## Prerequisites

| Tool | Purpose |
|------|---------|
| `nasm` | Boot + user program assembly |
| `qemu-system-x86_64` | Run the Grid |
| `x86_64-elf-gcc` (macOS) or `gcc` (Linux) | Build kernel + programs |
| `python3` | Seed Flynn arcade disk |

## First boot

```bash
git clone https://github.com/17060/grid-os.git
cd grid-os
make disk seed-disk
make run          # GUI window (macOS cocoa / Linux gtk)
```

You land in the **GridBASIC IDE**. Press **Esc** to open the embedded `grid>` shell.

### Display modes

| Command | Resolution |
|---------|------------|
| `make run` | Default window, zoom-to-fit |
| `make run-hd` | 1920×1080 HDMI (scaled VGA text) |
| `make run-4k` | 3840×2160 HDMI (scaled VGA text) |
| `make run-headless` | Serial-only shell (no window) |

## Hello GridBASIC

1. Boot with `make run` — IDE opens with a blank buffer.
2. Type:
   ```basic
   10 PRINT "hello grid"
   20 END
   ```
3. Press **Esc**, type `:run`, Enter.
4. Press **Esc**, type `:save hello`, Enter — writes `/programs/hello.bas`.
5. Later from `grid>`: `basic run /programs/hello.bas`

Try the seeded samples:

```text
grid> basic run /programs/hello.bas
grid> basic run /programs/netdemo.bas
grid> basic run /programs/vaultdemo.bas
```

### Useful IDE commands

| Command | Action |
|---------|--------|
| `:run` / `:r` | Run buffer |
| `:save <name>` | Save to GFS |
| `:load <name>` | Load from GFS |
| `:list` | Print program |
| `:ai explain` | Explain current line |
| `:vault list` | List vault keys (inline) |
| `:vault put key value` | Store in vault |
| `:vault sync` | Persist vault to disk |

**Up/Down** at the `grid>` prompt recalls command history.

## Flynn shell essentials

```text
grid> help
grid> status
grid> ls /
grid> cat /programs/hello.bas
grid> spawn lightcycle    # WASD game in ring 3
grid> net ping gateway    # ping QEMU gateway (DNS name)
grid> http get gateway /  # HTTP/1.1 with keep-alive pool
grid> poweroff            # exit QEMU
```

## IRC chat

Guest IP: **10.0.2.15**, gateway: **10.0.2.2** (alias: `gateway`)

```text
grid> irc connect gateway 6667 gridbot
grid> irc join #gridos
grid> irc say #gridos hello from Grid OS
grid> irc read
```

Or from IDE: `:irc connect gateway 6667 gridbot`

See [NETWORKING.md](NETWORKING.md) for concurrent TCP sessions (IRC + HTTP + AI + BTC together).

## Grid AI (host bridge)

**Terminal 1:**
```bash
make ai-bridge    # TCP :8766 → Ollama or OpenAI-compatible API
```

**Terminal 2:**
```bash
make run
```

In IDE: `:ai ask write a for loop`  
In shell: `ai ask explain PRINT`  
In BASIC: `PRINT GRID.AI.ASK$("how do I loop?")`

## Grid BTC (host bridge)

**Terminal 1:**
```bash
export BITCOIN_RPC_URL=http://127.0.0.1:8332/
export BITCOIN_RPC_USER=...
export BITCOIN_RPC_PASSWORD=...
make btc-bridge    # TCP :8767 → Bitcoin Core RPC
```

Use **testnet/regtest** for development.

```text
grid> btc status
grid> btc balance
grid> btc info
```

## HTTP fetch

Fetch a page over plain HTTP (port 80):

```text
grid> http get gateway /
grid> http get 10.0.2.2 /
```

Uses HTTP/1.1 with a keep-alive connection pool. TLS is not supported in-guest — use host bridges for encrypted services.

## Vault persistence

```text
grid> vault put motd "The Grid is open"
grid> vault get motd
grid> vault sync          # write to arcade disk
```

Host backup:

```bash
./tools/gridctl backup              # creates grid-os-backup-YYYYMMDD-HHMMSS.tar.gz
./tools/gridctl backup mygrid.tar.gz
```

Restores kernel source, disk image, and build artifacts. Re-seed with `make disk seed-disk` if you need a fresh Flynn archive.

## Mac Silicon distribution

On a Mac with Homebrew:

```bash
make release-mac
# → dist/grid-os-macos-arm64-v6.5.tar.gz
# → dist/GridOS-6.5-macOS-AppleSilicon.command
```

Upload to GitHub releases with `gh release upload v6.5 dist/*`

## Host tools (`gridctl`)

```bash
./tools/gridctl status
./tools/gridctl disk && ./tools/gridctl seed
./tools/gridctl install lightcycle
./tools/gridctl run-4k
./tools/gridctl test
./tools/gridctl backup
```

## Run tests

```bash
make test
```

Runs:

| Target | Coverage |
|--------|----------|
| `test-host-basic` | GridBASIC math, `:=`, `;`, status string |
| `test-host-vault` | v5 migration, full vault, genome parse |
| `test-host-vault-disk` | Simulated v5 disk → v6 round-trip |
| `test-host-tcp` | Dual/triple port dispatch, 8-slot limit |
| `test-host-net` | Static DNS resolver |
| `test-host-spawn` | Sandbox spawn fault regression |
| `test-qemu-smoke` | Boot stability |
| `test-e2e` | basictest → spawn gridsh → poweroff |

CI runs the same on every push to `main` via GitHub Actions.

## What's not in-guest yet

- TLS/HTTPS (use host bridges)
- Full Bitcoin node or LLM in-kernel (128 MB RAM — bridges instead)
- Real DNS (static hostname table only)

**4K display:** use `make run-4k` — see [DISPLAY_4K.md](DISPLAY_4K.md).

See `docs/COMMANDS.md` for the complete command reference.

**Security labs:** [docs/SECURITY_LABS.md](SECURITY_LABS.md) — 700 hat-color GridBASIC demos (`redteam`, `greyteam`, …).

**GridBASIC encyclopedia:** [docs/wiki/README.md](wiki/README.md) — colon commands, keywords, `GRID.*`, and all seeded modules.

**Cookbook:** [docs/wiki/cookbook.md](wiki/cookbook.md) — run modules, install packages, compile bytecode, earn disc XP.
