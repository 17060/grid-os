# Getting started

**Welcome:** [Introduction to Grid OS & the GridBASIC IDE](../INTRODUCTION.md)

## What is the GridBASIC IDE?

The **GridBASIC IDE** is the fullscreen editor built into Grid OS. You write programs in **GridBASIC** (an advanced BASIC dialect), run them immediately, save to Flynn disk (`/programs/*.bas`), compile to bytecode (`.grid`), and call Grid OS services through **`GRID.*` bindings**.

Enter from the Flynn shell:

```text
grid> basic
grid> basic ide
grid> basic ide /programs/hello.bas
```

Grid OS boots into the IDE by default after the banner.

## Three modes

| Mode | How | Prompt / action |
|------|-----|-----------------|
| **Edit** | Default | Type on program lines |
| **Colon commands** | Press `Esc`, type `:command` | Runs IDE-specific actions |
| **Shell** | Press `Esc`, type without `:` | Full Flynn `grid>` shell |

Press `Esc` on an empty command line to cancel.

## Your first program

1. Boot Grid OS (or type `basic` at `grid>`).
2. Type in the editor:

```basic
10 PRINT "=== Flynn's Grid ==="
20 PRINT GRID.STATUS$
30 FOR I = 1 TO 3
40   PRINT "line "; I
50 NEXT I
60 END
```

3. Press `Esc`, type `:run`, press Enter.
4. Press any key to return to the editor.

## Save and reload

```text
Esc :save hello        → writes /programs/hello.bas
Esc :load hello        → reads it back
Esc :list              → print listing fullscreen
Esc :new               → clear buffer
```

## Run samples and tutorial

```text
Esc :samples           → list Flynn disk samples
Esc :tutorial          → interactive walkthrough
grid> tutorial         → run /programs/tutorial.bas (from shell)
```

## Package modules (25 tools)

Special-purpose scripts shipped in **`flynn-ide-tools`**:

```text
Esc :mods              → list all modules
Esc :mod run grid-ping → run network ping panel
Esc :mod load ide-cheatsheet → open module source in editor
grid> pkg mods
grid> basic mod run disc-status
```

See [Package modules](package-modules.md).

## Compile to bytecode

```text
Esc :compile myprog    → /programs/myprog.grid
grid> basic compile /programs/hello.bas /programs/hello.grid
```

Run bytecode with `basic run /programs/myprog.grid`.

## Get help inside the IDE

| Command | Result |
|---------|--------|
| `Esc :help` | IDE help screen (keys + colon commands + language summary) |
| `Esc help` | Full Flynn shell command list |
| `grid> basic help` | GridBASIC version + syntax summary |

## Exit Grid OS

```text
Esc poweroff
```

(`:quit` only shows a hint — use `poweroff`.)

## Next steps

- [Editor keys](editor-keys.md)
- [Colon commands](colon-commands.md)
- [Statements](statements.md)
- [GRID bindings](grid-bindings.md)
