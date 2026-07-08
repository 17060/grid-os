# Grid OS Security Labs

**750 GridBASIC demos** across ten labs on the Flynn arcade disk — nine hat-color security teams plus **Flynn daemon** background-service demos for the GridBASIC IDE. Each lab teaches a slice of Grid OS — capabilities, vault, GFS, networking, host bridges, audit logs, IDE daemons — in a controlled QEMU environment.

**Run labs in QEMU only.** GridBASIC in the kernel is trusted code with broad capabilities. These demos are for learning and purple-team drills, not production attack tooling.

## Quick start

```bash
make disk seed-disk   # builds Flynn disk + seeds all labs
make run              # boot into GridBASIC IDE
```

Press **Esc** for the `grid>` shell, then:

```text
grid> greyteam                              # list all grey-hat demos
grid> basic run /programs/greyteam/gy01-probe-1.bas
grid> basic run /programs/whiteteam/wt97-list-security-labs.bas
```

From the IDE colon bar:

```text
:greyteam
:run /programs/purpleteam/pt01-vault-canary.bas
```

Each lab also has a **menu** on disk: `/programs/<lab>/menu.bas`.

## Hat colors at a glance

| Hat | Lab | Prefix | Demos | Role |
|-----|-----|--------|-------|------|
| Red | `redteam` | `rt` | 100 | Authorized offensive recon — caps, GFS, vault, net |
| Black | `blackhat` | `bh` | 100 | Malicious patterns — exfil, persistence, log forge |
| White | `whiteteam` | `wt` | 100 | Ethical hardening, compliance, authorized testing |
| Blue | `blueteam` | `bt` | 100 | SOC detection — audit watch, vault/GFS IOCs, net monitor |
| Purple | `purpleteam` | `pt` | 25 | Chained attack → detect → fix in one program |
| Green | `greenteam` | `gt` | 75 | DevSecOps — CI, seeding, path hygiene, lab cleanup |
| Yellow | `yellowteam` | `yt` | 50 | Audit, policy, GRC, compliance review |
| Orange | `orangeteam` | `ot` | 50 | Threat intel — IOCs, net/GFS/BTC/AI collection |
| Grey | `greyteam` | `gy` | 100 | Ambiguous ethics — probe, disclose, compare hats |
| Flynn | `daemonteam` | `dm` | 50 | **IDE background daemons** — jobs, audit, net, vault, spawn |

**Shell commands:** `redteam`, `blackhat`, `whiteteam`, `blueteam`, `purpleteam`, `greenteam`, `yellowteam`, `orangeteam`, `greyteam`, `daemonteam`

**IDE colon commands:** `:redteam`, `:blackhat`, … `:greyteam`, `:daemonteam` (same listings)

## Lab catalog

### Red team (`rt01`–`rt100`)

First 25 demos are hand-crafted core scenarios (`rt01-caps`, `rt06-vault-canary`, `rt10-http-get`, …). Extended demos cover vault keys, GFS paths, HTTP endpoints, ping targets, BTC RPC, AI prompts, and combo recon scripts.

**Start here:** `rt01-caps.bas` → `rt05-vault-dump.bas` → `rt24-full-recon.bas`

### Black hat (`bh01`–`bh100`)

Offensive patterns: vault exfil, GFS drops (`/programs/blackhat/drop-NN.txt`), log markers, persistence keys (`bh-persist-NN`), HTTP abuse, AI/BTC misuse.

Blue team demos **watch for these artifacts** after you run black/red exercises.

### White team (`wt01`–`wt100`)

Defensive posture: read-only cap audits, hardening messages, integrity scans, bridge audits.

**List all labs:** `wt97-list-security-labs.bas` — prints GFS listings for all nine `/programs/<lab>/` directories.

### Blue team (`bt01`–`bt100`)

SOC-style monitoring: `GRID.LOG.TAIL$`, vault IOC checks for `bh-persist-*`, GFS drop watchers, net anomaly baselines, BTC/AI bridge watches.

Run **after** red/black demos to practice detection.

### Purple team (`pt01`–`pt25`)

Each demo is a **three-act chain** in one file:

1. **Attack** — simulated red/black action  
2. **Detect** — blue-style observation  
3. **Fix** — white-style cleanup  

Examples: `pt01-vault-canary`, `pt03-gfs-drop`, `pt13-btc-bridge`, `pt25-graduation`.

### Green hat (`gt01`–`gt75`)

DevSecOps for Grid OS itself: seed-disk hygiene, GFS path checks, package MANIFEST review, lab canary cleanup, `make test` reminders.

### Yellow hat (`yt01`–`yt50`)

Audit and compliance: policy statements, vault/GFS review, bridge records, GRC inventory, graduation capstone.

### Orange hat (`ot01`–`ot50`)

Threat intel collection: audit-log IOCs, DNS/ping intel, GFS path profiling (unique path-based filenames for `ot21`–`ot30`), BTC method calls, AI surface mapping, intel packs fed to red/blue.

### Grey hat (`gy01`–`gy100`)

Ambiguous-ethics scenarios by tier:

| Demos | Theme |
|-------|-------|
| `gy01`–`gy10` | Unauthorized-style probes |
| `gy11`–`gy20` | Vault gray reads |
| `gy21`–`gy30` | GFS gray reads (paths across labs) |
| `gy31`–`gy40` | HTTP gray enumeration |
| `gy41`–`gy50` | Gray ping scans |
| `gy51`–`gy60` | BTC gray RPC |
| `gy61`–`gy70` | AI gray prompts |
| `gy71`–`gy80` | Gray persistence (`gy-gray-NN` vault keys) |
| `gy81`–`gy90` | Gray log markers |
| `gy91`–`gy100` | Combos — disclose, compare hats, purple bridge, graduation |

**Capstones:** `gy94-disclose.bas`, `gy99-cleanup-gray.bas`, `gy100-graduation.bas`

## Suggested campaigns

### 1. First hour (basics)

```text
grid> basic run /programs/redteam/rt01-caps.bas
grid> basic run /programs/redteam/rt05-vault-dump.bas
grid> basic run /programs/blueteam/bt01-audit-watch-1.bas
grid> basic run /programs/whiteteam/wt97-list-security-labs.bas
```

### 2. Attack → detect (red + blue)

```text
grid> basic run /programs/redteam/rt06-vault-canary.bas
grid> basic run /programs/blueteam/bt61-detect-redteam-canary.bas
grid> basic run /programs/redteam/rt13-log-forge.bas
grid> basic run /programs/blueteam/bt01-audit-watch-1.bas
```

### 3. Full purple chain

```text
grid> purpleteam
grid> basic run /programs/purpleteam/pt01-vault-canary.bas
grid> basic run /programs/purpleteam/pt03-gfs-drop.bas
grid> basic run /programs/purpleteam/pt25-graduation.bas
```

### 4. Grey hat disclosure arc

```text
grid> basic run /programs/greyteam/gy01-probe-1.bas
grid> basic run /programs/greyteam/gy94-disclose.bas
grid> basic run /programs/greyteam/gy99-cleanup-gray.bas
grid> basic run /programs/greyteam/gy100-graduation.bas
```

### 5. Intel → action (orange → red/blue)

```text
grid> basic run /programs/orangeteam/ot41-ioc-pack.bas
grid> basic run /programs/redteam/rt24-full-recon.bas
grid> basic run /programs/blueteam/bt92-ioc-hunt.bas
```

### 6. Flynn daemon IDE lab

```text
grid> daemonteam
grid> basic run /programs/daemonteam/dm01-boot-daemon.bas
grid> basic run /programs/daemonteam/dm11-audit-tail-daemon.bas
grid> basic run /programs/daemonteam/dm41-spawn-daemon.bas
grid> basic run /programs/daemonteam/dm50-graduation.bas
```

From IDE: **Esc** `:daemonteam` then `:load dm01-boot-daemon` or `:run /programs/daemonteam/dm01-boot-daemon.bas`

## Host bridges (optional)

Some demos call host services. Without bridges they still run but may print offline/skip messages.

| Bridge | Command | Used by |
|--------|---------|---------|
| AI | `make ai-bridge` | `GRID.AI.*`, gy61–gy70, ot36–ot40 |
| Bitcoin | `make btc-bridge` | `GRID.BTC.*`, gy51–gy60, ot31–ot35 |
| HTTPS | `make https-bridge` | Extended HTTP scenarios |

Run bridges in separate terminals while QEMU is up.

## Runtime artifacts

Several demos **create lab canaries** on the Flynn disk or in vault memory:

| Artifact | Created by | Watched by |
|----------|------------|------------|
| `redteam-canary` vault key | `rt06`, purple chains | blue `bt61+`, white `wt` |
| `bh-persist-NN` vault keys | black hat demos | blue vault IOC demos |
| `/programs/blackhat/drop-NN.txt` | black hat GFS writes | blue GFS IOC demos |
| `/programs/redteam/canary.txt` | `rt23` | blue GFS canary checks |
| `gy-gray-NN` vault keys | `gy71`–`gy80` | `gy99-cleanup-gray` |

Use `greenteam` / `gt` demos or manual vault sync to clean up between sessions. Re-seed for a fresh disk:

```bash
make seed-disk
```

## Developer guide

### Generate demos

```bash
make gen-security-demos
```

Source: `tools/gen_security_demos.py` — regenerates all `.bas` files and `menu.bas` per lab under `programs/*/`.

### Audit before seed

```bash
make audit-security-demos
```

Checks demo counts, duplicate filenames, GFS READ paths against the seed inventory, and inode budget (760 / 1024 slots after full seed).

`make seed-disk` runs **gen → audit → seed** automatically.

### Layout on disk

| Path | Contents |
|------|----------|
| `/programs/redteam/` | 100 + menu |
| `/programs/blackhat/` | 100 + menu |
| `/programs/whiteteam/` | 100 + menu |
| `/programs/blueteam/` | 100 + menu |
| `/programs/purpleteam/` | 25 + menu |
| `/programs/greenteam/` | 75 + menu |
| `/programs/yellowteam/` | 50 + menu |
| `/programs/orangeteam/` | 50 + menu |
| `/programs/greyteam/` | 100 + menu |
| `/programs/daemonteam/` | 50 + menu |

Flynn disk: **128 MB**, **1280 GFS inodes** (`kernel/gfs.c`, `Makefile` `DISK_MB`).

### Tests

```bash
make test-host-basic
make test-host-pp
make test-e2e          # IDE :run, net ping, spawn gridsh
make audit-security-demos
```

## Security model reminder

| Layer | Trust |
|-------|-------|
| GridBASIC in kernel IDE | Trusted — full `GRID.*` capabilities |
| Ring-3 spawn (`gridsh`, `lightcycle`, …) | Sandboxed user programs |
| Host bridges (AI, BTC, HTTPS) | Trust boundary at `10.0.2.2` |

Red/black/grey demos exercise the **trusted BASIC surface** deliberately so defenders can learn what kernel-level programs can do. For untrusted code, use `spawn` sandboxes — see [GETTING_STARTED.md](GETTING_STARTED.md).

## Related docs

- [GETTING_STARTED.md](GETTING_STARTED.md) — boot, IDE, samples  
- [COMMANDS.md](COMMANDS.md) — full shell reference  
- [NETWORKING.md](NETWORKING.md) — DNS, HTTP, bridges  
- [docs/wiki/grid-bindings.md](wiki/grid-bindings.md) — all `GRID.*` APIs used in demos  
- [VISION.md](VISION.md) — Grid OS security philosophy  
