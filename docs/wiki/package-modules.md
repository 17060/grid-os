# Package modules

The **`flynn-ide-tools`** package (v2.0) ships **25 GridBASIC IDE modules** — small programs for Flynn Grid tasks. They install on Flynn disk under `/packages/flynn-ide-tools/`.

## Run a module

| From | Command |
|------|---------|
| IDE | `Esc :mod run <name>` |
| IDE | `Esc :mod load <name>` — open source in editor |
| Shell | `basic mod run <name>` |
| Shell | `basic mod load <name>` — open IDE with source |
| Shell | `pkg mods` — list all |
| GridBASIC | `GRID.PKG.MOD.RUN "<name>"` |

## Module index

<!-- AUTO:MODULE_TABLE:BEGIN -->
| Package | Module | Category | Path | Description |
|---------|--------|----------|------|-------------|
| `flynn-ide-tools` | `disc-status` | `disc` | `/packages/flynn-ide-tools/modules/disc-status.bas` | Identity disc status panel |
| `flynn-ide-tools` | `grid-ping` | `network` | `/packages/flynn-ide-tools/modules/grid-ping.bas` | Ping gateway and grid hosts |
| `flynn-ide-tools` | `patrol-arm` | `patrol` | `/packages/flynn-ide-tools/modules/patrol-arm.bas` | Start recognizer patrol |
| `flynn-ide-tools` | `patrol-stand-down` | `patrol` | `/packages/flynn-ide-tools/modules/patrol-stand-down.bas` | Stop recognizer patrol |
| `flynn-ide-tools` | `whoami-panel` | `disc` | `/packages/flynn-ide-tools/modules/whoami-panel.bas` | Entity type and identity |
| `flynn-ide-tools` | `caps-panel` | `system` | `/packages/flynn-ide-tools/modules/caps-panel.bas` | Granted capability mask |
| `flynn-ide-tools` | `net-status` | `network` | `/packages/flynn-ide-tools/modules/net-status.bas` | Virtio-net link status |
| `flynn-ide-tools` | `dns-lookup` | `network` | `/packages/flynn-ide-tools/modules/dns-lookup.bas` | Resolve Flynn host names |
| `flynn-ide-tools` | `vault-nodes` | `storage` | `/packages/flynn-ide-tools/modules/vault-nodes.bas` | List vault key nodes |
| `flynn-ide-tools` | `gfs-programs` | `storage` | `/packages/flynn-ide-tools/modules/gfs-programs.bas` | List Flynn /programs archive |
| `flynn-ide-tools` | `jobs-monitor` | `system` | `/packages/flynn-ide-tools/modules/jobs-monitor.bas` | Background sandbox jobs |
| `flynn-ide-tools` | `iso-roster` | `system` | `/packages/flynn-ide-tools/modules/iso-roster.bas` | ISO research zone entities |
| `flynn-ide-tools` | `audit-tail` | `system` | `/packages/flynn-ide-tools/modules/audit-tail.bas` | Recent audit log entries |
| `flynn-ide-tools` | `grid-clock` | `grid` | `/packages/flynn-ide-tools/modules/grid-clock.bas` | Grid cycle timer ticks |
| `flynn-ide-tools` | `grid-clear` | `grid` | `/packages/flynn-ide-tools/modules/grid-clear.bas` | Clear screen with Flynn banner |
| `flynn-ide-tools` | `pkg-index` | `storage` | `/packages/flynn-ide-tools/modules/pkg-index.bas` | Installed packages and modules |
| `flynn-ide-tools` | `sample-menu` | `dev` | `/packages/flynn-ide-tools/modules/sample-menu.bas` | GridBASIC sample program guide |
| `flynn-ide-tools` | `ide-cheatsheet` | `dev` | `/packages/flynn-ide-tools/modules/ide-cheatsheet.bas` | IDE colon-command reference |
| `flynn-ide-tools` | `beep-scale` | `grid` | `/packages/flynn-ide-tools/modules/beep-scale.bas` | PC speaker note demo |
| `flynn-ide-tools` | `plot-grid` | `grid` | `/packages/flynn-ide-tools/modules/plot-grid.bas` | VGA plot pattern demo |
| `flynn-ide-tools` | `ai-ask` | `bridge` | `/packages/flynn-ide-tools/modules/ai-ask.bas` | Quick AI bridge question |
| `flynn-ide-tools` | `btc-snapshot` | `bridge` | `/packages/flynn-ide-tools/modules/btc-snapshot.bas` | Bitcoin bridge status |
| `flynn-ide-tools` | `irc-check` | `network` | `/packages/flynn-ide-tools/modules/irc-check.bas` | IRC session status |
| `flynn-ide-tools` | `hosts-table` | `network` | `/packages/flynn-ide-tools/modules/hosts-table.bas` | Show /etc/hosts from Flynn disk |
| `flynn-ide-tools` | `spawn-catalog` | `system` | `/packages/flynn-ide-tools/modules/spawn-catalog.bas` | Ring-3 program spawn hints |
| `flynn-net-tools` | `http-probe` | `network` | `/packages/flynn-net-tools/modules/http-probe.bas` | HTTP GET probe via GRID.HTTP |
| `flynn-net-tools` | `irc-connect` | `network` | `/packages/flynn-net-tools/modules/irc-connect.bas` | IRC quick-connect helper |
| `flynn-net-tools` | `https-bridge` | `bridge` | `/packages/flynn-net-tools/modules/https-bridge.bas` | HTTPS bridge status (host bridge) |
| `flynn-net-tools` | `grid-server` | `network` | `/packages/flynn-net-tools/modules/grid-server.bas` | TCP line server with custom keywords |
<!-- AUTO:MODULE_TABLE:END -->

---

## Encyclopedia entries

### `disc-status`

**Category:** Identity disc  
**Run:** `Esc :mod run disc-status`  
**Sample use:** Check disc level/XP before spawning programs.

```basic
10 REM IDE module: disc-status
20 GRID.CLS
30 PRINT "=== Identity Disc ==="
40 PRINT GRID.DISC.STATUS$
50 PRINT "Entity: "; GRID.DISC.ENTITY$
60 PRINT "Level: "; GRID.DISC.LEVEL
70 PRINT "XP: "; GRID.DISC.XP
80 END
```

---

### `grid-ping`

**Category:** Network  
**Run:** `Esc :mod run grid-ping`  
**Sample use:** Verify gateway reachability before HTTP/IRC demos.

```basic
10 PRINT "gateway: "; GRID.PING("gateway")
20 PRINT "grid: "; GRID.PING("grid")
30 PRINT "bridge: "; GRID.PING("bridge")
```

---

### `patrol-arm` / `patrol-stand-down`

**Category:** Recognizer  
**Run:** `Esc :mod run patrol-arm` then later `patrol-stand-down`  
**Sample use:** Start/stop background recognizer patrol from IDE.

---

### `whoami-panel` / `caps-panel`

**Category:** Security  
**Sample use:** Confirm entity type and capabilities before vault writes.

---

### `net-status` / `dns-lookup` / `hosts-table`

**Category:** Network  
**Sample use:** Debug networking before `GRID.HTTP.GET$` or IRC modules.

---

### `vault-nodes`

**Category:** Vault  
**Sample use:** Inspect vault keys before `GRID.VAULT.PUT`.

---

### `gfs-programs` / `sample-menu` / `spawn-catalog`

**Category:** Flynn disk / programs  
**Sample use:** Discover `.bas` samples and spawnable ELF programs.

---

### `jobs-monitor`

**Category:** Background jobs  
**Sample use:** After `GRID.SPAWN.BG "gridloop"`, run module to list jobs.

---

### `iso-roster`

**Category:** ISO zone  
**Sample use:** List research entities before `iso evolve` in shell.

---

### `audit-tail`

**Category:** Audit log  
**Sample use:** Review last 8 kernel log events after running tests.

---

### `grid-clock`

**Category:** Timer  
**Sample use:** Demonstrates `GRID.TIME` and `GRID.WAIT`.

---

### `grid-clear`

**Category:** Console  
**Sample use:** Clean screen with Flynn banner before demos.

---

### `pkg-index`

**Category:** Packages  
**Sample use:** Print `GRID.PKG.LIST$` and `GRID.PKG.MODS$`.

---

### `ide-cheatsheet`

**Category:** IDE help  
**Sample use:** Quick reference — run then `Esc :mod load ide-cheatsheet` to edit.

---

### `beep-scale` / `plot-grid`

**Category:** Sound / graphics  
**Sample use:** Demo `GRID.NOTE`, `GRID.BEEP`, `GRID.PLOT` in workshops.

---

### `ai-ask` / `btc-snapshot` / `irc-check`

**Category:** Host bridges  
**Requires:** `make ai-bridge` / `btc-bridge` on host (optional — offline fallbacks exist for AI).  
**Sample use:** Verify bridge connectivity from inside Grid OS.

---

## Install & author

```bash
python3 tools/gen_flynn_ide_modules.py   # regenerate MANIFEST + seeds
python3 tools/sync_basic_wiki.py         # refresh this table
make seed-disk
```

Push over GridLink:

```bash
./tools/gridctl portal-pkg-push packages/flynn-ide-tools/MANIFEST
# guest: portal pkg
```

See [PACKAGES.md](../PACKAGES.md).

## See also

- [Colon commands](colon-commands.md) → `:mods`, `:mod run`, `:mod load`
- [GRID bindings](grid-bindings.md) → `GRID.PKG.*`
