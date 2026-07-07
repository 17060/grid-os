# Grid OS 7.1 — Packages & IDE modules

Grid OS includes a lightweight package manager for Flynn disk (`GFS2FLYN`). Packages bundle GridBASIC **IDE modules** — special-purpose scripts you can run from the shell, IDE, or GridBASIC.

**Encyclopedia:** [wiki/README.md](wiki/README.md) · [Package modules](wiki/package-modules.md)

## Quick start

At the `grid>` prompt:

```text
pkg list                 # installed packages
pkg mods                 # all 25 IDE modules
basic mod run disc-status
basic mod load ide-cheatsheet   # opens GridBASIC IDE with module source
```

In the GridBASIC IDE, press **Esc** and type:

```text
:mods
:mod run patrol-arm
:mod load pkg-index
```

## Seeded package: flynn-ide-tools (v2.0)

25 modules for the GridBASIC IDE and Flynn shell environment:

| Module | Purpose |
|--------|---------|
| `disc-status` | Identity disc status panel |
| `grid-ping` | Ping gateway, grid, and bridge |
| `patrol-arm` | Start recognizer patrol |
| `patrol-stand-down` | Stop recognizer patrol |
| `whoami-panel` | Entity type and identity |
| `caps-panel` | Granted capability mask |
| `net-status` | Virtio-net link status |
| `dns-lookup` | Resolve Flynn host names |
| `vault-nodes` | List vault key nodes |
| `gfs-programs` | List Flynn `/programs` archive |
| `jobs-monitor` | Background sandbox jobs |
| `iso-roster` | ISO research zone entities |
| `audit-tail` | Recent audit log entries |
| `grid-clock` | Grid cycle timer ticks |
| `grid-clear` | Clear screen with Flynn banner |
| `pkg-index` | Installed packages and modules |
| `sample-menu` | GridBASIC sample program guide |
| `ide-cheatsheet` | IDE colon-command reference |
| `beep-scale` | PC speaker note demo |
| `plot-grid` | VGA plot pattern demo |
| `ai-ask` | Quick AI bridge question |
| `btc-snapshot` | Bitcoin bridge status |
| `irc-check` | IRC session status |
| `hosts-table` | Show `/etc/hosts` from Flynn disk |
| `spawn-catalog` | Ring-3 program spawn hints |

Files live under `/packages/flynn-ide-tools/` on the Flynn arcade disk.

Regenerate from source: `python3 tools/gen_flynn_ide_modules.py`

## MANIFEST format

Each package has a `MANIFEST` file:

```text
name=flynn-ide-tools
version=2.0
desc=25 GridBASIC IDE tools for Flynn's Grid
file=/packages/flynn-ide-tools/MANIFEST
file=/packages/flynn-ide-tools/modules/disc-status.bas
mod=disc-status:/packages/flynn-ide-tools/modules/disc-status.bas:Identity disc status panel
```

- **`name`**, **`version`**, **`desc`** — package metadata
- **`file=`** — files owned by the package (removed on `pkg remove`)
- **`mod=name:path:description`** — registers a GridBASIC IDE module

Install from an on-disk manifest:

```text
pkg install /packages/flynn-ide-tools/MANIFEST
```

Remove a package and its files:

```text
pkg remove flynn-ide-tools
```

## GridBASIC bindings

| Binding | Action |
|---------|--------|
| `GRID.PKG.LIST$` | Comma-separated package names |
| `GRID.PKG.MODS$` | Comma-separated module names |
| `GRID.PKG.INSTALL path$` | Register manifest |
| `GRID.PKG.REMOVE name$` | Uninstall package |
| `GRID.PKG.MOD.RUN name$` | Run module |
| `GRID.PKG.RECV` | Receive GridLink PKG frame |

Also: `GRID.PORTAL.PKG` (alias for package receive over GridLink).

## GridLink install (host → guest)

Host builds a PKG frame from a manifest:

```bash
python3 tools/gridpkg_build.py packages/flynn-ide-tools/MANIFEST
# or
./tools/gridctl portal-pkg-push packages/flynn-ide-tools/MANIFEST
```

Guest (while frame is piped to COM1):

```text
portal pkg
# or
pkg recv
```

## Authoring modules

1. Add `packages/flynn-ide-tools/modules/my-mod.bas` (GridBASIC source).
2. Add entries to `tools/gen_flynn_ide_modules.py` or edit `MANIFEST` manually.
3. Run `python3 tools/gen_flynn_ide_modules.py` to sync MANIFEST + `kernel/gfs.c` seeds.
4. `make seed-disk` — push with `gridctl portal-pkg-push` or `pkg install`.

Source template: [packages/flynn-ide-tools/](../packages/flynn-ide-tools/).
