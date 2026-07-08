# 02 — IDE colon commands

31 encyclopedia entries.

## `:run`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:run` |
| **Purpose** | Run buffer or GFS file |
| **Action** | Executes GridBASIC from editor or path |
| **Sample** | `programs/encyclopedia/ide-run.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :run
20 REM Where: IDE colon bar
30 REM Purpose: Run buffer or GFS file
40 REM Action: Executes GridBASIC from editor or path
50 PRINT "IDE-RUN-OK"
60 END
```

---

## `:save`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:save` |
| **Purpose** | Save buffer to Flynn disk |
| **Action** | Writes /programs/<name>.bas |
| **Sample** | `programs/encyclopedia/ide-save.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :save
20 REM Where: IDE colon bar
30 REM Purpose: Save buffer to Flynn disk
40 REM Action: Writes /programs/<name>.bas
50 PRINT "saved"
60 END
```

---

## `:load`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:load` |
| **Purpose** | Load program into editor |
| **Action** | Reads /programs/<name>.bas or .grid |
| **Sample** | `programs/encyclopedia/ide-load.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :load
20 REM Where: IDE colon bar
30 REM Purpose: Load program into editor
40 REM Action: Reads /programs/<name>.bas or .grid
50 PRINT GRID.STATUS$
60 END
```

---

## `:new`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:new` |
| **Purpose** | Clear editor buffer |
| **Action** | Resets buffer, path, dirty flag |
| **Sample** | `programs/encyclopedia/ide-new.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :new
20 REM Where: IDE colon bar
30 REM Purpose: Clear editor buffer
40 REM Action: Resets buffer, path, dirty flag
50 PRINT "fresh buffer"
60 END
```

---

## `:list`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:list` |
| **Purpose** | List buffer fullscreen |
| **Action** | Shows numbered program; key to return |
| **Sample** | `programs/encyclopedia/ide-list.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :list
20 REM Where: IDE colon bar
30 REM Purpose: List buffer fullscreen
40 REM Action: Shows numbered program; key to return
50 PRINT "line 1"
60 END
```

---

## `:compile`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:compile` |
| **Purpose** | Compile to bytecode |
| **Action** | Writes GRIDBC to /programs/<name>.grid |
| **Sample** | `programs/encyclopedia/ide-compile.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :compile
20 REM Where: IDE colon bar
30 REM Purpose: Compile to bytecode
40 REM Action: Writes GRIDBC to /programs/<name>.grid
50 PRINT "compile me from buffer"
60 END
```

---

## `:find`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:find` |
| **Purpose** | Search buffer |
| **Action** | Case-insensitive search from cursor |
| **Sample** | `programs/encyclopedia/ide-find.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :find
20 REM Where: IDE colon bar
30 REM Purpose: Search buffer
40 REM Action: Case-insensitive search from cursor
50 PRINT "search target"
60 END
```

---

## `:goto`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:goto` |
| **Purpose** | Jump to line |
| **Action** | Moves editor cursor to 1-based line |
| **Sample** | `programs/encyclopedia/ide-goto.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :goto
20 REM Where: IDE colon bar
30 REM Purpose: Jump to line
40 REM Action: Moves editor cursor to 1-based line
50 PRINT "line 1"
60 PRINT "line 2"
70 END
```

---

## `:mods`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:mods` |
| **Purpose** | List IDE modules |
| **Action** | Runs pkg mods with optional filter |
| **Sample** | `programs/encyclopedia/ide-mods.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :mods
20 REM Where: IDE colon bar
30 REM Purpose: List IDE modules
40 REM Action: Runs pkg mods with optional filter
50 PRINT GRID.PKG.MODS$
60 END
```

---

## `:mod`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:mod` |
| **Purpose** | Run IDE module |
| **Action** | Executes package module fullscreen |
| **Sample** | `programs/encyclopedia/ide-mod-run.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :mod
20 REM Where: IDE colon bar
30 REM Purpose: Run IDE module
40 REM Action: Executes package module fullscreen
50 PRINT "use :mod run disc-status"
60 END
```

---

## `:mod`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:mod` |
| **Purpose** | Load module source |
| **Action** | Opens module .bas in editor |
| **Sample** | `programs/encyclopedia/ide-mod-load.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :mod
20 REM Where: IDE colon bar
30 REM Purpose: Load module source
40 REM Action: Opens module .bas in editor
50 PRINT "use :mod load ide-cheatsheet"
60 END
```

---

## `:pkg`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:pkg` |
| **Purpose** | List packages |
| **Action** | Shows installed Grid packages |
| **Sample** | `programs/encyclopedia/ide-pkg-list.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :pkg
20 REM Where: IDE colon bar
30 REM Purpose: List packages
40 REM Action: Shows installed Grid packages
50 PRINT GRID.PKG.LIST$
60 END
```

---

## `:pkg`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:pkg` |
| **Purpose** | List modules via :pkg |
| **Action** | Same as :mods |
| **Sample** | `programs/encyclopedia/ide-pkg-mods.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :pkg
20 REM Where: IDE colon bar
30 REM Purpose: List modules via :pkg
40 REM Action: Same as :mods
50 PRINT GRID.PKG.MODS$
60 END
```

---

## `:samples`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:samples` |
| **Purpose** | List sample programs |
| **Action** | Shell passthrough: samples |
| **Sample** | `programs/encyclopedia/ide-samples.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :samples
20 REM Where: IDE colon bar
30 REM Purpose: List sample programs
40 REM Action: Shell passthrough: samples
50 PRINT "Esc then :samples"
60 END
```

---

## `:tutorial`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:tutorial` |
| **Purpose** | Interactive tutorial |
| **Action** | Multi-step IDE walkthrough |
| **Sample** | `programs/encyclopedia/ide-tutorial.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :tutorial
20 REM Where: IDE colon bar
30 REM Purpose: Interactive tutorial
40 REM Action: Multi-step IDE walkthrough
50 PRINT "Esc then :tutorial"
60 END
```

---

## `:help`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:help` |
| **Purpose** | IDE help overlay |
| **Action** | Full command and language summary |
| **Sample** | `programs/encyclopedia/ide-help.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :help
20 REM Where: IDE colon bar
30 REM Purpose: IDE help overlay
40 REM Action: Full command and language summary
50 PRINT "Esc then :help"
60 END
```

---

## `:quit`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:quit` |
| **Purpose** | Quit hint |
| **Action** | Reminds to use poweroff; does not exit OS |
| **Sample** | `programs/encyclopedia/ide-quit.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :quit
20 REM Where: IDE colon bar
30 REM Purpose: Quit hint
40 REM Action: Reminds to use poweroff; does not exit OS
50 PRINT "use grid> poweroff"
60 END
```

---

## `:ai`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:ai` |
| **Purpose** | Ask Grid AI |
| **Action** | Sends prompt to host bridge or offline help |
| **Sample** | `programs/encyclopedia/ide-ai-ask.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :ai
20 REM Where: IDE colon bar
30 REM Purpose: Ask Grid AI
40 REM Action: Sends prompt to host bridge or offline help
50 PRINT GRID.AI.ASK$("What is PRINT?", "ASK")
60 END
```

---

## `:ai`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:ai` |
| **Purpose** | Explain current line |
| **Action** | AI explains editor line under cursor |
| **Sample** | `programs/encyclopedia/ide-ai-explain.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :ai
20 REM Where: IDE colon bar
30 REM Purpose: Explain current line
40 REM Action: AI explains editor line under cursor
50 PRINT "move cursor then :ai explain"
60 END
```

---

## `:ai`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:ai` |
| **Purpose** | Complete buffer |
| **Action** | AI suggests completion for buffer |
| **Sample** | `programs/encyclopedia/ide-ai-complete.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :ai
20 REM Where: IDE colon bar
30 REM Purpose: Complete buffer
40 REM Action: AI suggests completion for buffer
50 PRINT "partial code then :ai complete"
60 END
```

---

## `:ai`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:ai` |
| **Purpose** | Fix GridBASIC code |
| **Action** | AI suggests corrected source |
| **Sample** | `programs/encyclopedia/ide-ai-fix.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :ai
20 REM Where: IDE colon bar
30 REM Purpose: Fix GridBASIC code
40 REM Action: AI suggests corrected source
50 PRINT "paste code to :ai fix"
60 END
```

---

## `:ai`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:ai` |
| **Purpose** | Show AI models |
| **Action** | Bridge model info |
| **Sample** | `programs/encyclopedia/ide-ai-models.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :ai
20 REM Where: IDE colon bar
30 REM Purpose: Show AI models
40 REM Action: Bridge model info
50 PRINT GRID.AI.MODELS$
60 END
```

---

## `:redteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:redteam` |
| **Purpose** | Red team lab menu |
| **Action** | Lists 100 offensive recon demos |
| **Sample** | `programs/encyclopedia/ide-redteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :redteam
20 REM Where: IDE colon bar
30 REM Purpose: Red team lab menu
40 REM Action: Lists 100 offensive recon demos
50 PRINT "Esc then :redteam"
60 END
```

---

## `:blackhat`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:blackhat` |
| **Purpose** | Black hat lab menu |
| **Action** | Lists 100 malicious-pattern demos |
| **Sample** | `programs/encyclopedia/ide-blackhat.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :blackhat
20 REM Where: IDE colon bar
30 REM Purpose: Black hat lab menu
40 REM Action: Lists 100 malicious-pattern demos
50 PRINT "Esc then :blackhat"
60 END
```

---

## `:whiteteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:whiteteam` |
| **Purpose** | White team lab menu |
| **Action** | Lists 100 ethical demos |
| **Sample** | `programs/encyclopedia/ide-whiteteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :whiteteam
20 REM Where: IDE colon bar
30 REM Purpose: White team lab menu
40 REM Action: Lists 100 ethical demos
50 PRINT "Esc then :whiteteam"
60 END
```

---

## `:blueteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:blueteam` |
| **Purpose** | Blue team lab menu |
| **Action** | Lists 100 SOC demos |
| **Sample** | `programs/encyclopedia/ide-blueteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :blueteam
20 REM Where: IDE colon bar
30 REM Purpose: Blue team lab menu
40 REM Action: Lists 100 SOC demos
50 PRINT "Esc then :blueteam"
60 END
```

---

## `:purpleteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:purpleteam` |
| **Purpose** | Purple team chains |
| **Action** | Lists 25 attack/detect/fix demos |
| **Sample** | `programs/encyclopedia/ide-purpleteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :purpleteam
20 REM Where: IDE colon bar
30 REM Purpose: Purple team chains
40 REM Action: Lists 25 attack/detect/fix demos
50 PRINT "Esc then :purpleteam"
60 END
```

---

## `:greenteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:greenteam` |
| **Purpose** | Green hat lab |
| **Action** | Lists 75 DevSecOps demos |
| **Sample** | `programs/encyclopedia/ide-greenteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :greenteam
20 REM Where: IDE colon bar
30 REM Purpose: Green hat lab
40 REM Action: Lists 75 DevSecOps demos
50 PRINT "Esc then :greenteam"
60 END
```

---

## `:yellowteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:yellowteam` |
| **Purpose** | Yellow hat lab |
| **Action** | Lists 50 audit demos |
| **Sample** | `programs/encyclopedia/ide-yellowteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :yellowteam
20 REM Where: IDE colon bar
30 REM Purpose: Yellow hat lab
40 REM Action: Lists 50 audit demos
50 PRINT "Esc then :yellowteam"
60 END
```

---

## `:orangeteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:orangeteam` |
| **Purpose** | Orange hat lab |
| **Action** | Lists 50 threat intel demos |
| **Sample** | `programs/encyclopedia/ide-orangeteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :orangeteam
20 REM Where: IDE colon bar
30 REM Purpose: Orange hat lab
40 REM Action: Lists 50 threat intel demos
50 PRINT "Esc then :orangeteam"
60 END
```

---

## `:greyteam`

| Field | Value |
|-------|-------|
| **Where** | IDE colon bar (Esc) |
| **Syntax** | `:greyteam` |
| **Purpose** | Grey hat lab |
| **Action** | Lists 100 gray-ethics demos |
| **Sample** | `programs/encyclopedia/ide-greyteam.bas` |
| **See also** | [colon-commands.md](../colon-commands.md) |

```basic
10 REM Encyclopedia: :greyteam
20 REM Where: IDE colon bar
30 REM Purpose: Grey hat lab
40 REM Action: Lists 100 gray-ethics demos
50 PRINT "Esc then :greyteam"
60 END
```

---
