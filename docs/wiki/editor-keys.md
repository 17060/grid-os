# Editor keys

All keys apply while the **editor** has focus (not while typing at `grid>`).

## Movement

| Key | Action |
|-----|--------|
| **Left / Right** | Move cursor within line |
| **Up / Down** | Move to previous / next line |
| **Home** | Start of line |
| **End** | End of line |

The editor scrolls so the cursor stays visible.

## Editing

| Key | Action |
|-----|--------|
| **Enter** | Split line — inserts newline below cursor |
| **Backspace** | Delete char left; at column 0, merge with line above |
| **Delete** | Delete char right; at end of line, merge with line below |
| **Tab** | Insert spaces (indent) |

## Command line

| Key | Action |
|-----|--------|
| **Esc** | Open `grid>` command line at bottom |
| **Esc** (empty prompt) | Cancel command, return to editor |
| **Enter** (at `grid>`) | Submit command |

Colon commands start with `:` after `Esc` (e.g. `:run`). Shell commands omit the colon (e.g. `pkg mods`).

## History

At the `grid>` prompt, **Up / Down** recall previous shell commands (shared with the main Flynn shell).

## Example session

1. Edit line 20 with arrows.
2. `Esc` → `:run` → Enter → program runs.
3. Any key → back to editor.
4. `Esc` → `pkg mods` → Enter → module list → any key → editor.

## See also

- [Getting started](getting-started.md)
- [Colon commands](colon-commands.md)
