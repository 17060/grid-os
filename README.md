# Grid OS 6.0

**Flynn's Grid** — a real x86_64 operating environment inspired by *Tron*. Open, creative, user-first. Not CLU's "perfect system."

## What's new in 6.0 — GridBASIC

- **Advanced BASIC language** — a real interpreter (`kernel/basic.c`) deeply interwoven into the Grid: a tokenizing lexer, recursive-descent expression evaluator, numeric + string values, arrays (`DIM`), and the classic control flow you'd expect.
- **Fullscreen IDE** (`kernel/basic_ide.c`) — a built-in editor with arrow-key cursor movement, Home/End, line split/merge, and a colon command bar (`:run`, `:save`, `:load`, `:new`, `:list`, `:help`, `:quit`). Programs are saved as text on GFS.
- **GridBASIC statements** — `PRINT`, `LET`, `IF/THEN/ELSE`, `FOR/TO/STEP/NEXT`, `WHILE/WEND`, `REPEAT/UNTIL`, `GOTO`, `GOSUB/RETURN`, `INPUT`, `DIM`, `REM`, `END/STOP`, with `?` as a `PRINT` shorthand.
- **Built-in functions** — `ABS INT SGN SQR RND LEN VAL ASC CHR$ STR$ UPPER$ LOWER$ LEFT$ RIGHT$ MID$ PI`, plus `+` string concatenation.
- **Grid bindings (deep interweaving)** — BASIC programs reach into the OS:
  - Statements: `GRID.CLS`, `GRID.COLOR n`, `GRID.LOG msg`, `GRID.WAIT ticks`, `GRID.SPAWN "name"`, `GRID.SERIAL.WRITE s$`
  - Functions: `GRID.TIME`, `GRID.RND(n)`, `GRID.PING(ip$)`, `GRID.STATUS$`, `GRID.SERIAL.READ$`
- **`basic` shell command** — `basic` (open IDE), `basic ide [file]`, `basic run <file>`, `basic help`.
- Sample program seeded at `/programs/hello.bas`.

## What's new in 5.1

- **IRC client** — `irc <server-ip> <port> <nick> <#channel>` joins a real IRC server over TCP
- **Minimal TCP stack** (`kernel/tcp.c`) — SYN/ACK/FIN state machine, sequence numbers, ACKs, on-the-wire checksums
- **ARP cache + raw IP send** — the gateway (10.0.2.2) is resolved once and reused as the L2 next-hop; QEMU user-net NATs outbound TCP to the internet
- **`irc` command** — registers (NICK/USER), joins a channel, answers PING/PONG, prints `PRIVMSG` lines, quits cleanly

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
./tools/gridctl run-headless     # serial shell
```

## Quick tour

```
status                # kernel, disk, network, input
net status            # virtio-net MAC, IP, packet counts
net ping 10.0.2.2     # ICMP echo to the QEMU gateway
irc 10.0.2.2 6667 gridtest #gridos   # join an IRC server over TCP
basic run /programs/hello.bas        # run a GridBASIC program
basic                                # open the GridBASIC IDE
spawn bg gridloop     # long-running preempt demo
jobs
wait                  # block until jobs finish (gridloop resumes across slices)
portal export         # GridLink vault frame on COM1
portal recv           # install /programs/* from host over serial
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
- [docs/VISION.md](docs/VISION.md)

## License

MIT — open source, Flynn-style.
