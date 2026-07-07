# GridBASIC IDE Encyclopedia

Complete reference for **Grid OS GridBASIC IDE** — colon commands, shell access from the editor, language keywords, built-ins, `GRID.*` bindings, preprocessor directives, and package modules.

<!-- AUTO:META:BEGIN -->
- **Grid OS version:** Grid OS 7.1.1
- **Packages:** 2 seeded (30 IDE modules total)
- **`flynn-ide-tools`** von=2.1 — 25 modules — 25 GridBASIC IDE tools for Flynn's Grid (7.1.1 categories)
- **`flynn-net-tools`** von=1.0 — 5 modules — Flynn network bridge helpers for GridBASIC IDE
- **Last synced by:** `python3 tools/sync_basic_wiki.py`
<!-- AUTO:META:END -->

## How to use this wiki

Each page lists entries in encyclopedia form:

| Field | Meaning |
|-------|---------|
| **Syntax** | What you type |
| **Where** | IDE colon bar (`Esc`), `grid>` shell, or program source |
| **Description** | What it does |
| **Example** | Copy-paste sample |
| **See also** | Related entries |

**Quick paths**

| I want to… | Start here |
|------------|------------|
| Learn the IDE | [Getting started](getting-started.md) |
| Run my program | [Colon commands](colon-commands.md) → `:run` |
| Use Flynn shell from IDE | [Shell from IDE](shell-from-ide.md) |
| Write GridBASIC code | [Statements](statements.md) · [Built-ins](builtins.md) |
| Call the Grid from BASIC | [GRID bindings](grid-bindings.md) |
| Run packaged tools | [Package modules](package-modules.md) |

## Wiki contents

| Volume | Topics |
|--------|--------|
| [Getting started](getting-started.md) | Boot, editor modes, first program |
| [Editor keys](editor-keys.md) | Cursor, Esc, history |
| [Colon commands](colon-commands.md) | `:run`, `:save`, `:mods`, `:ai`, … |
| [Shell from IDE](shell-from-ide.md) | Every `grid>` command usable in IDE |
| [Statements](statements.md) | `PRINT`, `FOR`, `SELECT CASE`, `SUB`, … |
| [Built-ins & operators](builtins.md) | `ABS`, `INSTR$`, `AND`, `MOD`, … |
| [Preprocessor](preprocessor.md) | `#IF`, `#INCLUDE`, `#ELSE`, `#ENDIF` |
| [GRID bindings](grid-bindings.md) | All `GRID.*` statements and functions |
| [Package modules](package-modules.md) | Seeded IDE modules (both packages) |
| [Cookbook](cookbook.md) | Recipes — modules, packages, bytecode, duels |

## Keeping this wiki current

After changing commands, keywords, bindings, or package modules:

```bash
python3 tools/sync_basic_wiki.py   # refresh version + module table
python3 tools/gen_flynn_ide_modules.py  # if editing module sources
make test
```

Canonical quick tables also live in [docs/COMMANDS.md](../COMMANDS.md) and [docs/PACKAGES.md](../PACKAGES.md).

## Related docs

- [GETTING_STARTED.md](../GETTING_STARTED.md) — full Grid OS walkthrough
- [PACKAGES.md](../PACKAGES.md) — package manager & GridLink install
- [COMMANDS.md](../COMMANDS.md) — flat command cheat sheet
