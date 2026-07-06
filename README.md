# Grid OS 6.9

**Flynn's Grid** — a real x86_64 operating environment inspired by *Tron*. Open, creative, user-first. Not CLU's "perfect system."

**New here?** See [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) for a full walkthrough (boot, GridBASIC, IRC, AI/BTC bridges, CI, backups).

**Apple Silicon Mac?** See [docs/MAC_SILICON.md](docs/MAC_SILICON.md) — `make save-macos-arm64` or `make release-mac` for distributable bundles.

**Windows?** See [docs/WINDOWS.md](docs/WINDOWS.md) — download `GridOS-*-Windows-x64.zip` or `make release-windows` to build bundles.

**Android (Termux)?** See [docs/ANDROID_TERMUX.md](docs/ANDROID_TERMUX.md) — download `GridOS-*-Android-Termux.sh` or `make release-termux`.

**Networking?** See [docs/NETWORKING.md](docs/NETWORKING.md) — multi-TCP, `/etc/hosts`, UDP DNS, HTTP GET/POST, HTTPS bridge.

## What's new in 6.9

Flynn Boot Experience — `autoexec.bas` welcome on boot (`vault put autoexec off` to skip), `tutorial` / `samples` shell commands, new samples (`tutorial.bas`, `subdemo.bas`, `grid2d.bas`), IDE `:samples`. See [CHANGELOG.md](CHANGELOG.md).

## What's new in 6.8

Android (Termux) release — `GridOS-*-Android-Termux.sh` headless launcher, full source zip, `docs/ANDROID_TERMUX.md`, `make release-termux`. See [CHANGELOG.md](CHANGELOG.md).

## What's new in 6.7

Full advanced GridBASIC — DEF FN, ELSEIF, ON ERROR/GOTO/GOSUB, OPTION BASE, SUB/FUNCTION/CALL/LOCAL, 2D arrays, CONTINUE, MIN/MAX/TRIM$, plus GRID.DNS/JOBS/ISO/VAULT.EXPORT bindings. See [CHANGELOG.md](CHANGELOG.md).

## What's new in 6.6

Advanced GridBASIC — `CONST`, `DATA`/`READ`/`RESTORE`, `RANDOMIZE`, `INSTR$`, `SELECT CASE`, `EXIT FOR`/`WHILE`, `LINE INPUT`, plus `GRID.VAULT.*`, `GRID.GFS.*`, `GRID.HTTP.*`, `GRID.LOCATE`, and `GRID.INKEY$`. See [CHANGELOG.md](CHANGELOG.md).

## What's new in 6.5.1

Patch release — audit fixes for PING, vault migration, HTTP keep-alive, DNS, IRC, and CI. See [CHANGELOG.md](CHANGELOG.md).

## What's new in 6.5

- **UDP DNS resolver** — queries QEMU gateway `10.0.2.2:53` after built-in names and GFS `/etc/hosts`
- **GFS `/etc/hosts`** — editable static hostname table (seeded on Flynn disk)
- **HTTP POST** — `http post <host> [port] <path> <body>` with keep-alive pool
- **HTTPS host bridge** — `make https-bridge` (TCP :8768) proxies guest HTTP to upstream TLS
- **`GRID.AI.PRINT` / `GRID.BTC.PRINT`** — full-length console output (bypasses 1024-char string cap)
- **Vault migration banner** — v5→v6 upgrade logs to console + audit trail on boot
- **Expanded e2e tests** — basictest, `net ping gateway`, spawn gridsh, clean poweroff
- **Sample library** — `/programs/aidemo.bas`, `/programs/httpdemo.bas`, `/etc/hosts`
- **macOS CI job** — builds release bundles on `macos-latest`
- Runtime banners updated to 6.5

## What's new in 6.4

- **Static DNS** — resolve `gateway`, `grid`, `localhost`, `ai`, `btc` (plus IPv4) in shell, HTTP, and `GRID.PING()`
- **HTTP/1.1 keep-alive** — connection pool reuses TCP sessions to the same host
- **8 concurrent TCP sessions** — IRC + HTTP + AI + BTC bridges can run together
- **Longer GridBASIC strings** — 1024-byte `value_t` buffer for AI/BTC responses
- **Expanded host tests** — vault disk round-trip, DNS, triple TCP, spawn fault regression
- **Sample programs** — `/programs/netdemo.bas`, `/programs/vaultdemo.bas`
- **`make release-mac`** — Mac Silicon tarball + standalone `.command` launcher
- **`make release-windows`** — Windows x64 zip + `GridOS.bat` double-click launcher
- **`make release-termux`** — Android Termux `.sh` launcher + source zip

## What's new in 6.3

- **Multi-connection TCP** — per-port dispatch; up to 8 concurrent outbound sessions
- **Vault v5→v6 migration** — auto-upgrades legacy on-disk vaults
- **Vault checksum fix** — saved vaults validate correctly on load
- **Input hardening** — HTTP, IRC, and serial import bounds checks
- **Host test suite** — GridBASIC, vault, and TCP unit tests

## What's new in 6.0 — GridBASIC

- **Advanced BASIC language** — a real interpreter (`kernel/basic.c`) deeply interwoven into the Grid: a tokenizing lexer, recursive-descent expression evaluator, numeric + string values, arrays (`DIM`), and the classic control flow you'd expect.
- **Fullscreen IDE** (`kernel/basic_ide.c`) — a built-in editor with arrow-key cursor movement, Home/End, line split/merge, and a colon command bar (`:run`, `:save`, `:load`, `:new`, `:list`, `:help`, `:quit`). Programs are saved as text on GFS.
- **GridBASIC statements** — `PRINT`, `LET`, `IF/THEN/ELSE`, `FOR/TO/STEP/NEXT`, `WHILE/WEND`, `REPEAT/UNTIL`, `GOTO`, `GOSUB/RETURN`, `INPUT`, `DIM`, `REM`, `END/STOP`, with `?` as a `PRINT` shorthand.
- **Built-in functions** — `ABS INT SGN SQR RND LEN VAL ASC CHR$ STR$ UPPER$ LOWER$ LEFT$ RIGHT$ MID$ PI`, plus `+` string concatenation.
- **Grid bindings (deep interweaving)** — BASIC programs reach into the OS:
  - Statements: `GRID.CLS`, `GRID.COLOR n`, `GRID.LOG msg`, `GRID.WAIT ticks`, `GRID.SPAWN "name"`, `GRID.SERIAL.WRITE s$`
  - Functions: `GRID.TIME`, `GRID.RND(n)`, `GRID.PING(ip$)`, `GRID.STATUS$`, `GRID.SERIAL.READ$`
  - AI: `GRID.AI.ASK$`, `GRID.AI.EXPLAIN$`, `GRID.AI.FIX$`, `GRID.AI.COMPLETE$`, `GRID.AI.MODELS$`
  - BTC: `GRID.BTC.CALL$`, `GRID.BTC.INFO$`, `GRID.BTC.BALANCE$`, `GRID.BTC.ADDRESS$`, `GRID.BTC.STATUS$`
- **Grid AI** — IDE `:ai` commands, shell `ai`, and GRID.AI.* bindings; host LLM via `make ai-bridge` (TCP :8766) with offline keyword fallback
- **Grid BTC** — IDE/shell `btc` commands and GRID.BTC.* bindings; host Bitcoin Core via `make btc-bridge` (TCP :8767) with clear offline errors
- **`basic` shell command** — `basic` (open IDE), `basic ide [file]`, `basic run <file>`, `basic help`.
- Sample program seeded at `/programs/hello.bas`.

## What's new in 5.1

- **IRC client** — persistent TCP IRC session from shell, GridBASIC IDE (`Esc` → `irc connect …`), and `GRID.IRC.*` bindings
- **Minimal TCP stack** (`kernel/tcp.c`) — SYN/ACK/FIN state machine, sequence numbers, ACKs, on-the-wire checksums
- **ARP cache + raw IP send** — the gateway (10.0.2.2) is resolved once and reused as the L2 next-hop; QEMU user-net NATs outbound TCP to the internet
- **`irc` command** — connect/join/say/read/status; legacy one-shot `irc ip port nick #chan`; PING/PONG; 16-line message queue

## What's new in 5.0

- **The Grid is online** — virtio-net PCI driver with ARP + ICMP; Grid OS answers pings and can ping out
- **`net` shell command** — `net status`, `net ping <ip>`, `net poll`
- **Auto-polling** — the idle loop drains the receive queue, so the Grid answers ARP/ICMP while you type
- QEMU boots with `-netdev user` + `virtio-net-pci`; guest IP is `10.0.2.15`, gateway `10.0.2.2`

## What's new in 4.0

- **True preemptive multitasking** — the timer ISR saves full user CPU state (GPRs + RIP/RSP/RFLAGS) and resumes it on the next slice; background jobs make forward progress across preempts instead of restarting
- **`gridloop`** — long-running ring-3 demo that proves resume works (counts to 4,000,000 across preempts)
- **`programs` shows `resumed`** when a job has saved context from a timer preempt

## What's new in 3.0

- **GFS2FLYN** — 64 files, 16 KB each, on a 16 MB arcade disk
- **Host install** — `make install-prog PROG=gridsh` / `gridctl install gridsh` without re-seeding
- **GridLink portal** — framed vault export/import and live program install over COM1
- **Shell history** — Up/Down arrows recall prior commands at the `grid>` prompt

## What's new in 2.7

- **`wait`** — block until background jobs complete
- **Ctrl+C** — cancel the current shell line
- **`make test` / `gridctl test`** — smoke test without disk lock issues

## Run

**GUI (recommended for Workbench):**

```bash
cd ~/Projects/grid-os
make disk seed-disk
make run
```

**4K HDMI display (scaled VGA in a 3840×2160 window):**

```bash
make run-4k
```

Grid OS still renders at the classic **80×25 VGA text** console inside the guest. There is no physical HDMI port in QEMU — `make run-4k` opens a **3840×2160 cocoa window** with `zoom-to-fit=on`, so the text console scales up to fill the display. QEMU’s VGA device reports 4K via EDID; on macOS the launcher also tries to resize the window (grant **Accessibility** to Terminal/Cursor if the window stays small — you can drag-resize manually with zoom-to-fit enabled).

**Terminal-only (shell over serial):**

```bash
make run-headless
```

**Legacy IDE disk (older QEMU setups):**

```bash
make run-legacy
```

Host helper:

```bash
./tools/gridctl status
./tools/gridctl seed
./tools/gridctl install gridsh   # update one program on disk
./tools/gridctl run              # GUI
./tools/gridctl run-4k           # 4K HDMI window (scaled VGA)
./tools/gridctl run-headless     # serial shell
```

## Quick tour

```
status                # kernel, disk, network, input
net status            # virtio-net MAC, IP, packet counts
net ping 10.0.2.2     # ICMP echo to the QEMU gateway
irc connect 10.0.2.2 6667 gridbot   # connect persistent IRC session
irc join #gridos                  # join channel
irc say #gridos hello from Grid OS
irc read                          # print queued messages
basic                             # open GridBASIC IDE (Esc → irc connect …)
basic run /programs/hello.bas     # run a GridBASIC program
ai ask write a for loop              # Grid AI (offline or with bridge)
poweroff              # exit QEMU cleanly
ide                   # Grid Workbench (needs GUI — use make run)
```

## Host program install

After building user programs:

```bash
make all
make install-prog PROG=lightcycle
# or
./tools/gridctl install lightcycle
```

Live install while Grid OS runs (serial):

```bash
# terminal 1
make run-headless
# in Grid OS: portal recv
# terminal 2
./tools/gridctl portal-push /programs/lightcycle build/lightcycle.elf | ...
```

## Grid AI (LLM bridge)

Grid OS cannot run LLMs in-kernel. A host bridge forwards prompts to OpenAI-compatible APIs or local Ollama.

**Terminal 1 — start the bridge:**

```bash
# Ollama (default: http://127.0.0.1:11434/v1, model llama3.2)
make ai-bridge

# OpenAI-compatible
export OPENAI_API_KEY=sk-...
export GRIDAI_API_URL=https://api.openai.com/v1
export GRIDAI_MODEL=gpt-4o-mini
make ai-bridge
```

**Terminal 2 — run Grid OS:**

```bash
make run
```

In the GridBASIC IDE (boots by default), press **Esc** and type:

```
:ai ask write a for loop that prints 1 to 5
:ai explain
:ai complete
```

Or at the embedded `grid>` prompt: `ai ask ...`, `ai explain PRINT I`, `ai models`.

From GridBASIC source:

```basic
10 PRINT GRID.AI.ASK$("how do I loop?")
20 PRINT GRID.AI.EXPLAIN$("FOR I = 1 TO 5")
30 END
```

Without the bridge, offline keyword help still works for `PRINT`, `FOR`, `IF`, `GRID.*`, etc.

Protocol: framed `GRIDAI/1.0/<ACTION>` lines over TCP `10.0.2.2:8766` (QEMU gateway) or COM1 serial.

## Grid BTC (host bridge)

Grid OS cannot run Bitcoin Core in-kernel (128 MB RAM). Instead, a host bridge forwards JSON-RPC to Bitcoin Core on the host:

```bash
# On the host — use testnet/regtest; never expose RPC to the internet
export BITCOIN_RPC_USER=...
export BITCOIN_RPC_PASSWORD=...
make btc-bridge
```

In the GridBASIC IDE (Esc → `grid>`):

```
btc status
btc info
btc balance
btc send bc1q... 0.001
btc call getpeerinfo
```

From GridBASIC:

```basic
10 PRINT GRID.BTC.STATUS$()
20 PRINT GRID.BTC.INFO$()
30 PRINT GRID.BTC.BALANCE$()
40 END
```

Protocol: framed `GRIDBTC/1.0/<METHOD>` + optional JSON params over TCP `10.0.2.2:8767`. Without the bridge, responses explain how to start `make btc-bridge`.

## Ring-3 programs

| Program | Path | Description |
|---------|------|-------------|
| gridsh | `/programs/gridsh` | Interactive ring-3 shell |
| discinfo | `/programs/discinfo` | Identity disc record |
| gridprog | `/programs/gridprog` | Minimal sandbox demo |
| gridloop | `/programs/gridloop` | Long-running preempt demo |
| lightcycle | `/programs/lightcycle` | Light cycle v2 (WASD) |

## GridFS paths

```
/flynn/             Flynn archive (on disk)
/source/            GridScript source (on disk)
/programs/          ring-3 binaries (on disk)
/grid/              system logs (on disk)
/vault/<key>        vault node
/isos/              ISO research zone
```

## See also

- [CHANGELOG.md](CHANGELOG.md)
- [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md)
- [docs/NETWORKING.md](docs/NETWORKING.md)
- [docs/VISION.md](docs/VISION.md)

## License

MIT — open source, Flynn-style.
