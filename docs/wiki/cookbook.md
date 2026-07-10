# GridBASIC Cookbook

Practical recipes for Flynn's Grid — modules, packages, bytecode, and duels.

## Security lab campaign

750 lab demos ship on the Flynn disk (ten labs including Flynn daemonteam). From shell:

```text
grid> whiteteam
grid> basic run /programs/purpleteam/pt01-vault-canary.bas
grid> basic run /programs/greyteam/gy94-disclose.bas
```

See [SECURITY_LABS.md](../SECURITY_LABS.md) for hat colors, suggested campaigns, and developer workflow (`make audit-security-demos`).

## Run a seeded IDE module

From the Flynn shell:

```text
grid> pkg mods
grid> basic mod run disc-status
```

From the IDE (Esc for `grid>`):

```text
grid> :mods network
grid> :mod run http-probe
grid> :pkg run grid-ping
```

Categories include `disc`, `network`, `grid`, `system`, `storage`, `patrol`, `bridge`, and `dev`.

## Filter modules by category

```text
grid> pkg mods network
grid> :mods bridge
```

Seeded packages:

| Package | Modules | Focus |
|---------|---------|-------|
| `flynn-ide-tools` v2.1 | 25 | Disc, grid, vault, patrol, IDE cheatsheets |
| `flynn-net-tools` v1.0 | 3 | HTTP probe, IRC helper, HTTPS bridge docs |

## Compile and run bytecode

1. Load or write a `.bas` program in the IDE.
2. `:compile demo` — writes `/programs/demo.grid`.
3. `:run demo.grid` — runs GRIDBC bytecode directly.

Or from shell:

```text
grid> basic compile /programs/hello.bas /programs/hello.grid
grid> basic run /programs/hello.grid
```

## Install a package over GridLink

Host:

```bash
make run-headless
# In another terminal:
./tools/gridctl portal-pkg-push packages/flynn-net-tools/MANIFEST | socat - /dev/ttyUSB0
```

Or publish a frame file:

```bash
./tools/gridctl portal-pkg-publish packages/flynn-net-tools/MANIFEST
# -> dist/flynn-net-tools.gridpkg
```

Guest (while waiting on COM1):

```text
grid> portal pkg
grid> pkg list
grid> pkg mods network
```

## Earn identity disc XP

| Action | XP |
|--------|-----|
| Run any GridBASIC program | +1 |
| Run `/programs/*.bas` via `basic run` | +5 |
| Run an IDE module (`basic mod run`) | +2 |
| GridLink duel ping (`portal duel`) | +10 |

Check progress:

```text
grid> basic mod run disc-status
grid> spawn gridsh
gridsh> disc
```

## Lightcycle duel over GridLink

```text
grid> portal duel
```

Sends a GridLink `/DUEL` frame and spawns the lightcycle sandbox. Host bridges can relay duel frames between Flynn nodes.

## Find and navigate in the IDE

```text
grid> :find PRINT
grid> :goto 42
```

`:find` searches from the current line downward (case-insensitive). `:goto` jumps to a 1-based line number.

## Wiki reference

Full command and keyword lists live in [`docs/wiki/README.md`](README.md).
