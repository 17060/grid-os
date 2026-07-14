# Flynn Everyday — GridBASIC like 1980

Grid OS **7.2.0** ships a full **everyday programming** pack: games, apps, type-ins, and a place for your own programs — boot-to-BASIC with a home-computer feel (Atari ST, Commodore, Apple II vibes) on Flynn's Grid.

## Boot experience (Flynn Boot 2.0)

On startup, `autoexec.bas` welcomes you and opens the **GridBASIC IDE** with a hint:

```
Esc :catalog :games :publish | tutorial
```

Disable the boot script anytime:

```
vault put autoexec off
```

## What's on disk

| Pack | Path | Count | Shell | IDE |
|------|------|-------|-------|-----|
| **Catalog** | `/programs/everyday.bas` | menu | `catalog` | `:catalog` |
| **Games** | `/programs/games/` | 20 | `games` | `:games` |
| **Apps** | `/programs/apps/` | 10 | `apps` | `:apps` |
| **Type-ins** | `/programs/typeins/` | 20 | `typeins` | `:typeins` |
| **Your work** | `/programs/mine/` | — | `mine` | `:mine` |
| **Academy** | `/programs/academy.bas` | menu | `academy` | `:academy` |

Each pack includes a `menu.bas` you can run with `basic run /programs/games/menu.bas`.

## Try a game in 30 seconds

1. Boot Grid OS (`make run` or `make run-4k`).
2. When the IDE opens, press **Esc**.
3. Type `:load games/g01-guess` and press Enter.
4. Type `:run` and play.

Or from the shell (Esc, then at `grid>`):

```
games
basic run /programs/games/g01-guess.bas
```

## Publish your program

Write code in the IDE, then:

```
Esc :publish hello
```

Your program is saved to `/programs/mine/hello.bas` and loads later as `:load mine/hello`.

## Flynn Academy (advanced)

Security labs (750 demos across 9 hat-color teams + daemon lab) are still on disk but moved under **Academy** in help text so beginners see games first:

```
academy
redteam
```

IDE: `:academy`, `:redteam`, etc.

## Regenerate & seed

Host-side generator (creates `.bas` files under `programs/`):

```bash
make gen-everyday
```

Full Flynn disk seed (includes everyday pack):

```bash
make seed-disk
```

## See also

- [COOKBOOK_VOL1.md](COOKBOOK_VOL1.md) — first programs walkthrough
- [GETTING_STARTED.md](GETTING_STARTED.md) — full Grid OS tour
- [SECURITY_LABS.md](SECURITY_LABS.md) — Academy lab catalog
