# GridBASIC Cookbook — Volume 1

**Flynn Everyday edition** — your first hour on the Grid.

## Recipe 1: Hello, Flynn

Press **Esc** in the IDE and type:

```
:load typeins/t01-hello
:run
```

Or type this yourself:

```basic
10 PRINT "HELLO, FLYNN!"
20 END
```

Save with `:save hello` or publish to your folder with `:publish hello`.

## Recipe 2: Explore the catalog

```
:catalog
```

Lists games, apps, type-ins, mine, and academy. From the shell (without IDE):

```
catalog
games
```

## Recipe 3: Number guess (g01)

```
:load games/g01-guess
:run
```

Type guesses when prompted. Study the source — `RANDOMIZE`, `INPUT`, `IF`, `FOR`.

## Recipe 4: Grid graphics

```
:load games/g04-plotdemo
:run
```

Uses `GRID.CLS`, `GRID.PLOT`, `GRID.LINE`. Try changing colors and coordinates.

## Recipe 5: Beep melody

```
:load games/g05-beepscale
:run
```

Chain `GRID.BEEP` and `GRID.WAIT` for simple sound effects.

## Recipe 6: Publish to mine/

1. Write a short program in the IDE.
2. `Esc :publish myfirst`
3. `Esc :load mine/myfirst`
4. `Esc :run`

List your folder: `:mine` or shell `mine`.

## Recipe 7: Apps pack

Utilities live under `/programs/apps/`:

```
:load apps/a03-notes
:run
```

Uses vault for a sticky note. Try `a09-ping` for network status.

## Recipe 8: Type-in library

Twenty one-liners and mini demos — perfect for magazine-style learning:

```
typeins
:load typeins/t06-sub
:run
```

## Recipe 9: Tutorial refresh

```
:tutorial
```

Flynn Boot walkthrough — PRINT, LET, FOR, strings, `GRID.WHOAMI$`.

## Recipe 10: When you're ready for Academy

```
:academy
```

Security labs for QEMU lab environments. Everyday users can ignore these until curious.

---

**End of line.** Share your `.bas` files — `:publish` makes you a Grid programmer.
