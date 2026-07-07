# Grid OS 7.1.1 — Packages & IDE modules

Grid OS includes a lightweight package manager for Flynn disk (`GFS2FLYN`). Packages bundle GridBASIC **IDE modules** — special-purpose scripts you can run from the shell, IDE, or GridBASIC.

**Encyclopedia:** [wiki/README.md](wiki/README.md) · [Package modules](wiki/package-modules.md) · [Cookbook](wiki/cookbook.md)

## Quick start

At the `grid>` prompt:

```text
pkg list                 # installed packages
pkg mods                 # all IDE modules (28 seeded)
pkg mods network         # filter by category
basic mod run disc-status
basic mod load ide-cheatsheet   # opens GridBASIC IDE with module source
```

In the GridBASIC IDE, press **Esc** and type:

```text
:mods network
:mod run patrol-arm
:pkg run http-probe
:pkg list
```

## Seeded packages

### flynn-ide-tools (v2.1)

25 modules for the GridBASIC IDE and Flynn shell environment. Categories: `disc`, `network`, `grid`, `system`, `storage`, `patrol`, `bridge`, `dev`.

| Module | Category | Purpose |
|--------|----------|---------|
| `disc-status` | disc | Identity disc status panel |
| `grid-ping` | network | Ping gateway, grid, and bridge |
| `patrol-arm` | patrol | Start recognizer patrol |
| `patrol-stand-down` | patrol | Stop recognizer patrol |
| `whoami-panel` | disc | Entity type and identity |
| `caps-panel` | system | Granted capability mask |
| `net-status` | network | Virtio-net link status |
| `dns-lookup` | network | Resolve Flynn host names |
| `vault-nodes` | storage | List vault key nodes |
| `gfs-programs` | storage | List Flynn `/programs` archive |
| `jobs-monitor` | system | Background sandbox jobs |
| `iso-roster` | system | ISO research zone entities |
| `audit-tail` | system | Recent audit log entries |
| `grid-clock` | grid | Grid cycle timer ticks |
| `grid-clear` | grid | Clear screen with Flynn banner |
| `pkg-index` | storage | Installed packages and modules |
| `sample-menu` | dev | GridBASIC sample program guide |
| `ide-cheatsheet` | dev | IDE colon-command reference |
| `beep-scale` | grid | PC speaker note demo |
| `plot-grid` | grid | VGA plot pattern demo |
| `ai-ask` | bridge | Quick AI bridge question |
| `btc-snapshot` | bridge | Bitcoin bridge status |
| `irc-check` | network | IRC session status |
| `hosts-table` | network | Show `/etc/hosts` from Flynn disk |
| `spawn-catalog` | system | Ring-3 program spawn hints |

### flynn-net-tools (v1.0)

| Module | Category | Purpose |
|--------|----------|---------|
| `http-probe` | network | HTTP GET probe via `GRID.HTTP` |
| `irc-connect` | network | IRC quick-connect helper |
| `https-bridge` | bridge | HTTPS bridge status (host bridge) |

Files live under `/packages/<name>/` on the Flynn arcade disk.

Regenerate from source: `python3 tools/gen_packages.py`

## MANIFEST format

Each package has a `MANIFEST` file:

```text
name=flynn-ide-tools
version=2.0
desc=25 GridBASIC IDE tools for Flynn's Grid
file=/packages/flynn-ide-tools/MANIFEST
file=/packages/flynn-ide-tools/modules/disc-status.bas
mod=disc-status:/packages/flynn-ide-tools/modules/disc-status.bas:Identity disc status panel:disc
```

- **`name`**, **`version`**, **`desc`** — package metadata
- **`file=`** — files owned by the package (removed on `pkg remove`)
- **`mod=name:path:description:category`** — registers a GridBASIC IDE module (category optional, defaults to `general`)

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
