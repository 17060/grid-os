# Grid OS — Changelog

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
