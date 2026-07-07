# Grid OS 7.1 — Packages & IDE modules

Grid OS includes a lightweight package manager for Flynn disk (`GFS2FLYN`). Packages bundle GridBASIC **IDE modules** — special-purpose scripts you can run from the shell, IDE, or GridBASIC.

## Quick start

At the `grid>` prompt:

```text
pkg list                 # installed packages
pkg mods                 # IDE modules (disc-status, grid-ping, patrol-arm)
basic mod run disc-status
basic mod load grid-ping   # opens GridBASIC IDE with module source
```

In the GridBASIC IDE, press **Esc** and type:

```text
:mods
:mod run patrol-arm
:mod load disc-status
```

## Seeded package: flynn-ide-tools

| Module | Purpose |
|--------|---------|
| `disc-status` | Identity disc status panel |
| `grid-ping` | Ping `gateway` and `grid` hosts |
| `patrol-arm` | Start recognizer patrol |

Files live under `/packages/flynn-ide-tools/` on the Flynn arcade disk.

## MANIFEST format

Each package has a `MANIFEST` file:

```text
name=flynn-ide-tools
version=1.0
desc=GridBASIC IDE tools for Flynn's Grid
file=/packages/flynn-ide-tools/MANIFEST
mod=disc-status:/packages/flynn-ide-tools/modules/disc-status.bas:Identity disc status panel
mod=grid-ping:/packages/flynn-ide-tools/modules/grid-ping.bas:Ping gateway and grid hosts
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

Frame format:

```text
#GRIDLINK/1.0/PKG
/packages/flynn-ide-tools/MANIFEST 386
...bytes...
/packages/flynn-ide-tools/modules/disc-status.bas 215
...bytes...
#GRIDLINK/END
```

## Authoring a package

1. Create `packages/my-pack/MANIFEST` and module `.bas` files under `packages/my-pack/`.
2. Push with `gridctl portal-pkg-push` while Grid OS runs `portal pkg`, **or** copy files to Flynn disk and run `pkg install /packages/my-pack/MANIFEST`.
3. List modules with `pkg mods` and run with `basic mod run <name>`.

Source template: [packages/flynn-ide-tools/](../packages/flynn-ide-tools/).
