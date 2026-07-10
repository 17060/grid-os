# 06 — GRID statements

39 encyclopedia entries.

## `GRID.CLS`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.CLS` |
| **Purpose** | Clear screen |
| **Action** | Clears console display |
| **Sample** | `programs/encyclopedia/grid-cls.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.CLS
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Clear screen
40 REM Action: Clears console display
50 GRID.CLS
60 PRINT "clean slate"
70 END
```

---

## `GRID.LOG`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.LOG msg$` |
| **Purpose** | Audit log entry |
| **Action** | Appends message to audit trail |
| **Sample** | `programs/encyclopedia/grid-log.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.LOG
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Audit log entry
40 REM Action: Appends message to audit trail
50 GRID.LOG "encyclopedia sample"
60 END
```

---

## `GRID.COLOR`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.COLOR n` |
| **Purpose** | Set text color |
| **Action** | Changes console color index |
| **Sample** | `programs/encyclopedia/grid-color.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.COLOR
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Set text color
40 REM Action: Changes console color index
50 GRID.COLOR 2
60 PRINT "colored"
70 END
```

---

## `GRID.LOCATE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.LOCATE row, col` |
| **Purpose** | Move cursor |
| **Action** | Positions console cursor |
| **Sample** | `programs/encyclopedia/grid-locate.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.LOCATE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Move cursor
40 REM Action: Positions console cursor
50 GRID.LOCATE 5, 10
60 PRINT "here"
70 END
```

---

## `GRID.WAIT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.WAIT ticks` |
| **Purpose** | Busy wait |
| **Action** | Delays for timer ticks |
| **Sample** | `programs/encyclopedia/grid-wait.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.WAIT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Busy wait
40 REM Action: Delays for timer ticks
50 GRID.WAIT 10
60 PRINT "done"
70 END
```

---

## `GRID.PRINT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PRINT expr` |
| **Purpose** | Print with newline |
| **Action** | Prints expression plus newline |
| **Sample** | `programs/encyclopedia/grid-print-grid.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PRINT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Print with newline
40 REM Action: Prints expression plus newline
50 GRID.PRINT "grid line"
60 END
```

---

## `GRID.SPAWN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.SPAWN name$` |
| **Purpose** | Spawn ring-3 |
| **Action** | Runs sandbox ELF by name |
| **Sample** | `programs/encyclopedia/grid-spawn.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.SPAWN
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Spawn ring-3
40 REM Action: Runs sandbox ELF by name
50 PRINT "use GRID.SPAWN "gridsh""
60 END
```

---

## `GRID.SPAWN.BG`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.SPAWN.BG name$` |
| **Purpose** | Background spawn |
| **Action** | Queues sandbox job |
| **Sample** | `programs/encyclopedia/grid-spawn-bg.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.SPAWN.BG
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Background spawn
40 REM Action: Queues sandbox job
50 PRINT "use GRID.SPAWN.BG "gridloop""
60 END
```

---

## `GRID.VAULT.PUT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.VAULT.PUT k$, v$` |
| **Purpose** | Vault store |
| **Action** | Sets vault key/value |
| **Sample** | `programs/encyclopedia/grid-vault-put.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.VAULT.PUT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Vault store
40 REM Action: Sets vault key/value
50 GRID.VAULT.PUT "enc", "demo"
60 GRID.VAULT.SYNC
70 END
```

---

## `GRID.VAULT.SYNC`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.VAULT.SYNC` |
| **Purpose** | Vault persist |
| **Action** | Writes vault to arcade disk |
| **Sample** | `programs/encyclopedia/grid-vault-sync.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.VAULT.SYNC
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Vault persist
40 REM Action: Writes vault to arcade disk
50 GRID.VAULT.SYNC
60 END
```

---

## `GRID.VAULT.EXPORT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.VAULT.EXPORT` |
| **Purpose** | Vault export |
| **Action** | Exports vault over COM1 |
| **Sample** | `programs/encyclopedia/grid-vault-export.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.VAULT.EXPORT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Vault export
40 REM Action: Exports vault over COM1
50 PRINT "requires COM1 bridge"
60 END
```

---

## `GRID.VAULT.IMPORT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.VAULT.IMPORT` |
| **Purpose** | Vault import |
| **Action** | Imports vault from COM1 |
| **Sample** | `programs/encyclopedia/grid-vault-import.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.VAULT.IMPORT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Vault import
40 REM Action: Imports vault from COM1
50 PRINT "requires COM1 bridge"
60 END
```

---

## `GRID.GFS.WRITE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.GFS.WRITE path$, data$` |
| **Purpose** | Write GFS file |
| **Action** | Creates/overwrites Flynn disk file |
| **Sample** | `programs/encyclopedia/grid-gfs-write.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.GFS.WRITE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Write GFS file
40 REM Action: Creates/overwrites Flynn disk file
50 GRID.GFS.WRITE "/programs/encyclopedia/tmp.txt", "hi"
60 END
```

---

## `GRID.JOBS.KILL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.JOBS.KILL n` |
| **Purpose** | Kill bg job |
| **Action** | Stops background job number |
| **Sample** | `programs/encyclopedia/grid-jobs-kill.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.JOBS.KILL
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Kill bg job
40 REM Action: Stops background job number
50 PRINT GRID.JOBS.LIST$
60 END
```

---

## `GRID.ISO.SPAWN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.ISO.SPAWN name$` |
| **Purpose** | Spawn ISO entity |
| **Action** | Seeds ISO research entity |
| **Sample** | `programs/encyclopedia/grid-iso-spawn.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.ISO.SPAWN
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Spawn ISO entity
40 REM Action: Seeds ISO research entity
50 PRINT GRID.ISO.LIST$
60 END
```

---

## `GRID.ISO.EVOLVE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.ISO.EVOLVE id` |
| **Purpose** | Evolve ISO |
| **Action** | Mutates ISO genome |
| **Sample** | `programs/encyclopedia/grid-iso-evolve.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.ISO.EVOLVE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Evolve ISO
40 REM Action: Mutates ISO genome
50 PRINT GRID.ISO.LIST$
60 END
```

---

## `GRID.SERIAL.WRITE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.SERIAL.WRITE expr` |
| **Purpose** | COM1 write |
| **Action** | Transmits text on serial port |
| **Sample** | `programs/encyclopedia/grid-serial-write.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.SERIAL.WRITE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: COM1 write
40 REM Action: Transmits text on serial port
50 GRID.SERIAL.WRITE "ping"
60 END
```

---

## `GRID.PLOT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PLOT x,y,c` |
| **Purpose** | Plot pixel |
| **Action** | Draws on VGA grid |
| **Sample** | `programs/encyclopedia/grid-plot.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PLOT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Plot pixel
40 REM Action: Draws on VGA grid
50 GRID.PLOT 10, 10, 2
60 END
```

---

## `GRID.LINE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.LINE x0,y0,x1,y1,c` |
| **Purpose** | Draw line |
| **Action** | Line on VGA grid |
| **Sample** | `programs/encyclopedia/grid-line.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.LINE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Draw line
40 REM Action: Line on VGA grid
50 GRID.LINE 0, 0, 20, 20, 1
60 END
```

---

## `GRID.CIRCLE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.CIRCLE cx,cy,r,c` |
| **Purpose** | Draw circle |
| **Action** | Circle on VGA grid |
| **Sample** | `programs/encyclopedia/grid-circle.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.CIRCLE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Draw circle
40 REM Action: Circle on VGA grid
50 GRID.CIRCLE 40, 40, 10, 3
60 END
```

---

## `GRID.BEEP`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BEEP freq, ms` |
| **Purpose** | Beep speaker |
| **Action** | Plays tone for milliseconds |
| **Sample** | `programs/encyclopedia/grid-beep.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BEEP
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Beep speaker
40 REM Action: Plays tone for milliseconds
50 GRID.BEEP 440, 100
60 END
```

---

## `GRID.NOTE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.NOTE n, ms` |
| **Purpose** | Play note |
| **Action** | Plays note number |
| **Sample** | `programs/encyclopedia/grid-note.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.NOTE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Play note
40 REM Action: Plays note number
50 GRID.NOTE 60, 100
60 END
```

---

## `GRID.RECOGNIZER.START`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.RECOGNIZER.START` |
| **Purpose** | Start patrol |
| **Action** | Starts recognizer background service |
| **Sample** | `programs/encyclopedia/grid-recog-start.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.RECOGNIZER.START
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Start patrol
40 REM Action: Starts recognizer background service
50 GRID.RECOGNIZER.START
60 PRINT GRID.RECOGNIZER.STATUS$
70 END
```

---

## `GRID.RECOGNIZER.STOP`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.RECOGNIZER.STOP` |
| **Purpose** | Stop patrol |
| **Action** | Stops recognizer service |
| **Sample** | `programs/encyclopedia/grid-recog-stop.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.RECOGNIZER.STOP
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Stop patrol
40 REM Action: Stops recognizer service
50 GRID.RECOGNIZER.STOP
60 END
```

---

## `GRID.PORTAL.RECV`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PORTAL.RECV` |
| **Purpose** | Portal recv file |
| **Action** | Receives GridLink program |
| **Sample** | `programs/encyclopedia/grid-portal-recv.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PORTAL.RECV
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Portal recv file
40 REM Action: Receives GridLink program
50 PRINT "needs host portal"
60 END
```

---

## `GRID.PORTAL.PKG`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PORTAL.PKG` |
| **Purpose** | Portal recv pkg |
| **Action** | Receives .gridpkg bundle |
| **Sample** | `programs/encyclopedia/grid-portal-pkg.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PORTAL.PKG
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Portal recv pkg
40 REM Action: Receives .gridpkg bundle
50 PRINT "needs host portal"
60 END
```

---

## `GRID.PORTAL.DUEL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PORTAL.DUEL` |
| **Purpose** | Lightcycle duel |
| **Action** | Portal duel ping + spawn |
| **Sample** | `programs/encyclopedia/grid-portal-duel.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PORTAL.DUEL
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Lightcycle duel
40 REM Action: Portal duel ping + spawn
50 PRINT "GridLink duel"
60 END
```

---

## `GRID.PKG.INSTALL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PKG.INSTALL path$` |
| **Purpose** | Install package |
| **Action** | Registers MANIFEST path |
| **Sample** | `programs/encyclopedia/grid-pkg-install.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PKG.INSTALL
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Install package
40 REM Action: Registers MANIFEST path
50 PRINT GRID.PKG.LIST$
60 END
```

---

## `GRID.PKG.REMOVE`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PKG.REMOVE name$` |
| **Purpose** | Remove package |
| **Action** | Uninstalls package |
| **Sample** | `programs/encyclopedia/grid-pkg-remove.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PKG.REMOVE
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Remove package
40 REM Action: Uninstalls package
50 PRINT GRID.PKG.LIST$
60 END
```

---

## `GRID.PKG.MOD.RUN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PKG.MOD.RUN name$` |
| **Purpose** | Run module |
| **Action** | Runs IDE module by name |
| **Sample** | `programs/encyclopedia/grid-pkg-mod-run.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PKG.MOD.RUN
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Run module
40 REM Action: Runs IDE module by name
50 PRINT GRID.PKG.MODS$
60 END
```

---

## `GRID.PKG.RECV`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PKG.RECV` |
| **Purpose** | Receive package |
| **Action** | COM1 GridLink PKG frame |
| **Sample** | `programs/encyclopedia/grid-pkg-recv.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PKG.RECV
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Receive package
40 REM Action: COM1 GridLink PKG frame
50 PRINT "needs COM1"
60 END
```

---

## `GRID.IRC.CONNECT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.IRC.CONNECT h$,p,n$` |
| **Purpose** | IRC connect |
| **Action** | Opens IRC TCP session |
| **Sample** | `programs/encyclopedia/grid-irc-connect.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.IRC.CONNECT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: IRC connect
40 REM Action: Opens IRC TCP session
50 PRINT GRID.IRC.STATUS$
60 END
```

---

## `GRID.IRC.JOIN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.IRC.JOIN chan$` |
| **Purpose** | IRC join |
| **Action** | JOIN channel |
| **Sample** | `programs/encyclopedia/grid-irc-join.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.IRC.JOIN
20 REM Where: GridBASIC GRID binding
30 REM Purpose: IRC join
40 REM Action: JOIN channel
50 PRINT GRID.IRC.STATUS$
60 END
```

---

## `GRID.IRC.SAY`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.IRC.SAY tgt$, msg$` |
| **Purpose** | IRC message |
| **Action** | Send PRIVMSG |
| **Sample** | `programs/encyclopedia/grid-irc-say.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.IRC.SAY
20 REM Where: GridBASIC GRID binding
30 REM Purpose: IRC message
40 REM Action: Send PRIVMSG
50 PRINT GRID.IRC.STATUS$
60 END
```

---

## `GRID.IRC.QUIT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.IRC.QUIT` |
| **Purpose** | IRC quit |
| **Action** | Disconnect IRC session |
| **Sample** | `programs/encyclopedia/grid-irc-quit.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.IRC.QUIT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: IRC quit
40 REM Action: Disconnect IRC session
50 PRINT GRID.IRC.STATUS$
60 END
```

---

## `GRID.BTC.SEND`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.SEND addr$, amt` |
| **Purpose** | Send bitcoin |
| **Action** | sendtoaddress via bridge |
| **Sample** | `programs/encyclopedia/grid-btc-send.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.SEND
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Send bitcoin
40 REM Action: sendtoaddress via bridge
50 PRINT GRID.BTC.STATUS$
60 END
```

---

## `GRID.AI.PRINT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.AI.PRINT prompt$ [, mode$]` |
| **Purpose** | Full AI output |
| **Action** | Prints full AI response to console |
| **Sample** | `programs/encyclopedia/grid-ai-print.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.AI.PRINT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Full AI output
40 REM Action: Prints full AI response to console
50 GRID.AI.PRINT "What is LET?", "EXPLAIN"
60 END
```

---

## `GRID.BTC.PRINT`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.PRINT method$ [, params$]` |
| **Purpose** | Full BTC output |
| **Action** | Prints full RPC response |
| **Sample** | `programs/encyclopedia/grid-btc-print.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.PRINT
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Full BTC output
40 REM Action: Prints full RPC response
50 GRID.BTC.PRINT "getnetworkinfo"
60 END
```

---

## `GRID.WORKSHOP.SPAWN`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.WORKSHOP.SPAWN name$` |
| **Purpose** | Workbench spawn |
| **Action** | Spawns from Grid Workbench |
| **Sample** | `programs/encyclopedia/grid-workshop-spawn.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.WORKSHOP.SPAWN
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Workbench spawn
40 REM Action: Spawns from Grid Workbench
50 PRINT "use ide/workshop"
60 END
```

---
