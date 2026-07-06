# Grid OS — Changelog

## 6.9 — Flynn Boot Experience

- **`/programs/autoexec.bas`** — welcome script runs once after boot banner (skip with `vault put autoexec off`)
- **`tutorial.bas`**, **`subdemo.bas`**, **`grid2d.bas`** — new seeded GridBASIC samples on Flynn disk
- **Shell** — `tutorial`, `samples`, `basic samples`; boot banner before GridBASIC IDE
- **IDE** — `:samples` lists Flynn disk programs
- **`hello.bas`** — version banner updated to 6.9
- Runtime banners updated to 6.9

## 6.8 — Android (Termux) release bundles

- **Android Termux launcher** — `GridOS-*-Android-Termux.sh` single-file boot (headless serial default)
- **`grid-os-android-termux-*.zip`** — full source + prebuilt kernel/disk for on-device builds
- **`docs/ANDROID_TERMUX.md`** — install, headless boot, Termux:X11 display, bridges
- **`make release-termux`** / `gridctl standalone-termux` — build bundles on Linux CI
- **CI** — `termux-release` job uploads Android artifacts

## 6.7 — Full advanced GridBASIC (Tier 1–3)

### Language (Tier 1)
- **DEF FN** — single-line user functions
- **ELSEIF** — multi-branch IF chains
- **ON GOTO / ON GOSUB** — computed dispatch
- **ON ERROR GOTO / RESUME** — error handlers with `ERR$`
- **OPTION BASE 0|1** — array indexing base
- **TRIM$ / LTRIM$ / RTRIM$**, **SPACE$**, **STRING$**
- **MIN / MAX / FIX / ROUND**

### Language (Tier 3)
- **SUB / FUNCTION … END SUB/FUNCTION** with **CALL** and **LOCAL**
- **CONTINUE FOR / CONTINUE WHILE**
- **2D arrays** — `DIM A(10,10)`, `A(i,j)` (string SELECT CASE already supported)

### GRID.* bindings (Tier 2)
- **GRID.DNS.RESOLVE$**, **GRID.NET.STATUS$**, **GRID.LOG.TAIL$(n)**
- **GRID.WHOAMI$**, **GRID.CAPS$**, **GRID.JOBS.LIST$ / KILL**, **GRID.ISO.LIST$ / SPAWN**
- **GRID.VAULT.EXPORT / IMPORT**, **GRID.SPAWN.BG**

### Kernel helpers
- `log_copy_tail()`, `sched_format_jobs()`, `iso_format_list()`, `net_format_status()`

## 6.6 — Advanced GridBASIC language + kernel bindings

- **Language** — `CONST`, `DATA`/`READ`/`RESTORE`, `RANDOMIZE`, `INSTR$`, `SELECT CASE`/`CASE`/`CASE ELSE`/`END SELECT`, `EXIT FOR`/`EXIT WHILE`, `LINE INPUT`
- **GRID bindings** — `GRID.VAULT.GET$`/`PUT`/`SYNC`/`LIST$`, `GRID.GFS.READ$`/`WRITE`/`LIST$`, `GRID.HTTP.GET$`/`POST$`, `GRID.LOCATE`, `GRID.INKEY$`
- **`GRID.CAP(n)`** — now calls real `security_has_capability()` instead of always returning 1
- **`storage_list_keys()`** — vault key listing for `GRID.VAULT.LIST$`
- **IDE** — syntax highlighting and `:help` text for new keywords
- **Sample** — `/programs/advancedemo.bas` seeded on Flynn disk
- **Host tests** — CONST, DATA/READ, RANDOMIZE, INSTR$ in `test-host-basic`

## 6.5.2 — Windows release bundles

- **Windows x64 bundles** — `make release-windows` produces `GridOS-*-Windows-x64.zip` (launcher) and `grid-os-windows-x64-*.zip` (full source)
- **`GridOS.bat`** — double-click launcher with QEMU discovery (winget/Chocolatey/Scoop paths)
- **`docs/WINDOWS.md`** — install, display modes, bridges, troubleshooting
- **`gridctl save-windows` / `standalone-windows`** — host helpers for Windows packaging
- **CI** — `windows-release` job builds zip artifacts on Ubuntu

## 6.5.1 — Audit fixes

- **`GRID.PING()`** — fixed inverted success/failure return (0 from `net_ping()` now maps to 1)
- **Vault v5→v6 migration** — disk sync no longer overwrites loaded vault identity via `storage_snapshot()`
- **HTTP client** — honors response `Content-Length` on keep-alive; correct `Host:` header from shell hostname
- **DNS** — bounds checks on UDP response parser; `/etc/hosts` reloads after GFS write
- **IRC** — `irc connect` resolves hostnames; no longer resets TCP stack (HTTP keep-alive preserved)
- **`net_ping()`** — gateway ARP wait uses `gateway_resolved`, not generic `arp_replies`
- **`net_send_ip()`** — rejects payloads that exceed virtio frame buffer
- **HTTPS bridge** — client socket timeout, header forwarding, accurate 502 `Content-Length`
- **Makefile** — preserve `-no-pie` on Linux CI; vault disk test depends on seeded image
- **Freestanding `memcpy`** — required for x86_64-elf-gcc macOS cross builds

## 6.5 — DNS, HTTP POST, HTTPS bridge, PRINT bindings, e2e

- **UDP DNS** — `dns_resolve()` sends A queries to `10.0.2.2:53`; integrated into `net_resolve_host()`
- **GFS `/etc/hosts`** — parser in `kernel/dns.c`; seeded at `/etc/hosts` on Flynn disk
- **HTTP POST** — `http_post()` / `http post` shell command; optional port on GET/POST
- **HTTPS bridge** — `tools/gridhttps_bridge.py` + `make https-bridge` (guest TCP → host TLS)
- **`GRID.AI.PRINT` / `GRID.BTC.PRINT`** — print full bridge responses to console
- **`GRID.AI.COMPLETE$` fix** — now calls `ai_complete()` instead of `ai_ask()`
- **Vault v5 migration message** — console + log banner on auto-upgrade at boot
- **E2E tests** — basictest, `net ping gateway`, spawn gridsh, poweroff (`make test-e2e`)
- **Samples** — `aidemo.bas`, `httpdemo.bas`, `/etc/hosts`
- **macOS CI** — release bundle job on GitHub Actions
- **Bridge version sweep** — Grid AI bridge system prompt updated to 6.5
- Runtime banners updated to 6.5

## 6.4 — Docs, DNS, HTTP keep-alive, tests, samples

- **Static DNS** — `net_resolve_host()` resolves `gateway`, `grid`, `localhost`, `ai`, `btc` (case-insensitive) plus literal IPv4; used by `http get`, `GRID.PING()`, and shell
- **HTTP/1.1 keep-alive pool** — reuses TCP connections across requests to the same host; `http_close_idle()` tears down the pool
- **TCP slots** — increased from 4 to 8 concurrent outbound sessions
- **GridBASIC strings** — `value_t.s` expanded to 1024 bytes for longer AI/BTC results
- **Host tests** — vault disk round-trip, DNS resolver, triple TCP dispatch, spawn fault regression script
- **CI** — split build/seed/host/QEMU steps; runs full host test matrix
- **Samples** — `/programs/netdemo.bas`, `/programs/vaultdemo.bas` seeded on Flynn disk
- **Docs** — README, GETTING_STARTED, COMMANDS, MAC_SILICON, and new NETWORKING.md updated to 6.4
- **`make release-mac`** — builds Mac Silicon tarball + standalone `.command` for release upload
- Runtime banners updated to 6.4

## 6.3 — Multi-TCP, vault migration, tests, hardening

- **Multi-connection TCP** — up to 4 concurrent outbound TCP sessions with per-connection local ports and tuple-based dispatch (IRC + HTTP + AI + BTC can run together)
- **Vault v5→v6 migration** — automatically loads legacy 2-sector v5 vaults, zero-fills the tail, re-checksums, and re-saves as v6
- **Input hardening** — HTTP rejects overlong paths and oversized headers; IRC reports truncated lines; serial import validates DISC hex length and ISO genome byte pairs
- **Host test suite** — `test-host-basic` (GridBASIC math, `:=`, trailing comma, status), `test-host-vault` (v5 migration, full vault, genome parse), `test-host-tcp` (dual-port dispatch, slot limit)
- **Vault checksum validation** — compare against a zeroed checksum field so saved vaults actually validate on load
- Runtime banners updated to 6.3

## 6.2 — Deep audit bug fixes

- **TCP handler registration** — `tcp_init()` now registers `tcp_input` at boot, so HTTP/AI/BTC (not just IRC) actually receive inbound segments
- **`tcp_recv` data loss** — no longer zeroes `rx_len` on entry; returns already-buffered data instead of dropping it
- **TCP overflow ACK** — only ACKs bytes actually copied and flags `error` when the RX buffer overflows
- **Vault disk layout** — bumped to vault v6 (3 disk sectors); the struct was ~1028 bytes but only 1024 were persisted, truncating the last entry
- **`storage_put` errors** — returns `-1` when the vault is full instead of silently succeeding
- **ISO genome import** — serial import reads all 32 genome bytes (was stopping at 30)
- **Sandbox spawn faults** — `program_spawn` / `program_spawn_elf` return `-1` and release the slot on fault instead of reporting success
- **ELF W+X rejection** — user segments can no longer be mapped writable *and* executable
- **HTTP request length** — rejects paths that would overflow/truncate the request buffer
- **GridBASIC** — trailing `PRINT` comma no longer emits an extra newline; `:=` assignment accepted; empty-identifier out-of-bounds guard; token-limit error reported; `value_t` string buffer grown to 512 bytes so AI/BTC results are not truncated at 160
- **IDE syntax highlighting** — keyword matching is now case-insensitive
- Version banners updated to 6.2

## 6.0 — GridBASIC advanced language + IDE

- **GridBASIC interpreter** (`kernel/basic.c`) — tokenizing lexer, recursive-descent expression evaluator, numeric + string values with `+` concatenation, one-dimensional arrays (`DIM`), and full classic control flow: `PRINT` (`?`), `LET`, `IF/THEN/ELSE`, `FOR/TO/STEP/NEXT`, `WHILE/WEND`, `REPEAT/UNTIL`, `GOTO`, `GOSUB/RETURN`, `INPUT`, `REM`, `END/STOP`
- **Built-in functions** — `ABS INT SGN SQR RND LEN VAL ASC CHR$ STR$ UPPER$ LOWER$ LEFT$ RIGHT$ MID$ PI`
- **Grid bindings** — BASIC programs call into the OS: `GRID.CLS`, `GRID.COLOR`, `GRID.LOG`, `GRID.WAIT`, `GRID.SPAWN`, `GRID.SERIAL.WRITE` (statements) and `GRID.TIME`, `GRID.RND`, `GRID.PING`, `GRID.STATUS$`, `GRID.SERIAL.READ$` (functions)
- **Fullscreen IDE** (`kernel/basic_ide.c`) — in-OS editor: arrow/Home/End cursor, line split & merge, backspace/delete, tab, and a colon command bar (`:run :save :load :new :list :help :quit`); programs persist as text on GFS
- **`basic` shell command** — `basic`, `basic ide [file]`, `basic run <file>`, `basic help`
- **Console key API** — `console_read_key()` plus Left/Right/Home/End/Delete scancodes for the editor
- Sample program seeded at `/programs/hello.bas`; double-precision math compiled with SSE for the interpreter/IDE only (public API has no doubles, rest of kernel stays `-mno-sse`)

## 5.1 — IRC client + minimal TCP

- **TCP stack** (`kernel/tcp.c`) — outbound client state machine: SYN → SYN-ACK → ACK, PSH/ACK data send, ACK on receive, FIN close; sequence-number tracking and correct TCP checksums (pseudo-header)
- **ARP cache + `net_send_ip`** — gateway MAC resolved once and reused as next-hop for all outbound IP; QEMU user-net NATs to the internet
- **IRC client** (`kernel/irc.c`) — `irc <ip> <port> <nick> <#channel>`: NICK/USER registration, JOIN, automatic PING/PONG, `PRIVMSG` parsing/printing, QUIT
- **`irc` shell command** — connects to a real IRC server and prints channel traffic for a bounded session
- TCP inbound dispatched from `net_poll` via `net_set_tcp_input`

## 5.0 — The Grid goes online

- **virtio-net driver** (`kernel/net.c`) — legacy PCI transport, RX/TX virtqueues, MAC read from device config
- **ARP + ICMP stack** — answers ARP requests for `10.0.2.15` and replies to ICMP echo requests; `net ping <ip>` sends ARP + ICMP echo
- **`net` command** — `net status` (MAC, IP, packet counts), `net ping <ip>`, `net poll`
- **Idle auto-poll** — `console_idle` drains the RX queue so the Grid responds to pings while at the prompt
- **QEMU networking** — `make run`/`run-headless`/`test` now attach `-netdev user` + `virtio-net-pci`
- **`status`** shows Network line

## 4.0 — True preemptive multitasking

- **Real context save/restore** — the timer ISR (`boot/interrupts.s`) now saves all GPRs plus RIP/RSP/RFLAGS into the running program's `user_ctx_t` and returns to the kernel; `program_run` resumes via `enter_usermode_resume` instead of restarting at the entry point
- **Forward progress across preempts** — background jobs advance across timer quanta instead of restarting from the beginning each slice (the 2.2 "preemptive" claim is now honest)
- **`gridloop`** — new ring-3 program that counts to 4,000,000, printing progress; completes only because preemption preserves its state
- **`programs` list** — shows `resumed` when a job has saved context from a preempt
- Removed the old fake-preempt path (`preempt_only`, `sched_run_pending` driven restart)

## 3.0 — GFS2FLYN + GridLink + host install

- **GFS2FLYN** — 64 inodes, 16 KB files, 16 MB disk (`GFS2FLYN` magic, version 2)
- **Host install** — `make install-prog PROG=name` and `gridctl install name` write ELF to `/programs/*`
- **GridLink portal** — `portal export|import|recv` framed protocol on COM1; `gridctl portal-push`
- **Shell history** — Up/Down arrow keys at the Flynn shell prompt
- **Honest scheduler note** — background jobs still run cooperatively while the shell waits; true CPU-state resume is planned for a future release

## 2.7 — Wait + Ctrl+C

- **`wait`** — block until all background jobs finish
- **Ctrl+C** — cancel the current shell line (PS/2 or serial)
- **`make test`** — uses disk snapshot to avoid QEMU image lock conflicts

## 2.6 — Foreground jobs + smoke test

- **`fg <#>`** — detach a background job and run it in the foreground
- **`programs`** — list now shows idle/running/exited/fault state per slot
- **`make test`** — headless boot smoke test (`status` + `poweroff`)

## 2.5 — Job control + clean shutdown

- **`kill <#>`** — stop a background sandbox job (`jobs` shows slot numbers)
- **`kill all`** — clear every queued background job
- **`poweroff` / `halt`** — clean QEMU exit via isa-debug-exit (port 0x501)

## 2.4 — Serial shell + headless mode

- **Serial mirror** — Flynn shell output copied to COM1 (`status`, banner, commands)
- **Serial input** — type at the `grid>` prompt over `-serial stdio`
- **`make run-headless`** — q35 + virtio, no GUI window (terminal-only shell)
- **`gridctl run-headless`** — host wrapper for headless boot
- Linker cleanup — multiboot header folded into the main LOAD segment

## 2.3 — Modern device compatibility

- **Virtio block disk** — PCI/MMIO `virtio-blk-pci` on q35 (auto-fallback to legacy IDE)
- **Extended memory map** — 128 MB identity map + MMIO mapping for PCI devices
- **Modern QEMU profile** — `make run` uses q35, virtio-blk, GUI display, 128 MB RAM
- **Legacy profile** — `make run-legacy` keeps IDE for older setups
- **Keyboard shift** — uppercase and shifted symbols on PS/2 keyboards
- **PS/2 mouse** — improved init for QEMU i8042 on q35

## 2.2 — Hardware keys + live cycles + background jobs

- **PS/2 mouse** in Grid Workbench desktop — pointer cursor and icon clicks
- **Background jobs** — `spawn bg <name>` queues ring-3 programs; `jobs` lists them
- **Timer quantum** — yields sandbox time while shell/IDE waits (cooperative bg scheduling; true preemption landed in 4.0)
- **F1–F10 hardware keys** in Grid Workbench (PS/2 scancodes, no typing `F3`)
- **Esc** returns to GEM desktop from Workbench
- **Grid cycles** advance on timer (1/sec) even while waiting at prompts

## 2.1 — Grid Workbench audit fixes

- Fixed desktop icon picks wiping output (removed erroneous full redraw)
- Fixed `FILE SAVE AS` menu command parsing
- Fixed GridScript `PRINT`/`RUN` corrupting VGA scroll state
- Fixed shell cursor desync after Workbench exit (`console_reset_cursor`)
- Fixed empty-path edge case in volume resolution
- CLI commands (`DIR`, `TYPE`, etc.) now update window title and chrome correctly

## 2.1 — Grid Workbench (GEM + AmigaDOS)

- **Grid Workbench** — Atari GEM desktop + AmigaDOS CLI (`ide`)
- GEM menu bar, window title bar, file selector, desktop icons, status line
- AmigaDOS prompt `1.GridOS:source/>` with volume assigns and `DIR`/`CD`/`ED`/`EXEC`
- GridScript editor inside GEM window; sample `/source/welcome.grid`

## 2.0 — Real OS + Tron flavor

- **GFS1FLYN** on-disk filesystem (`kernel/gfs.c`) — inode table, fixed slots, paths like `/programs/gridsh`
- **ELF64 loader** (`kernel/elf.c`) — PT_LOAD segments with W^X; flat `.bin` fallback
- **Disk-backed spawn** — `program_spawn_from_disk()` reads `/programs/<name>` before embedded fallback
- **Host seed tool** — `make seed-disk` / `./tools/gridctl seed` writes ELF programs via `tools/gfs_seed.py`
- **Tron polish** — `recognizer`, `theme flynn|clu`, light cycle v2 with Recognizer drone
- **Shell 2.0** — GFS status in banner, `gfs` command, `ls /programs`

## 1.0 — Flynn's Grid

- GridFS virtual paths, audit log, PIT timer, ISO autopilot
- Interactive `gridsh`, `lightcycle`, vault persistence, `gridctl`

## 0.5 — Arcade disk

- IDE driver, vault sync, `gridsh`, `discinfo`

## 0.4 — Serial portal

- COM1, CRC Grid Vault, export/import

## 0.3 — Ring 3

- User sandboxes, W^X, syscalls, `gridprog`

## 0.2 — ISO zone

- Spawn, evolve, quarantine, genomes

## 0.1 — Boot

- Multiboot2 kernel, identity discs, capability security
