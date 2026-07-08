# 07 — GRID functions

43 encyclopedia entries.

## `GRID.TIME`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.TIME` |
| **Purpose** | Timer ticks |
| **Action** | Returns result of GRID.TIME |
| **Sample** | `programs/encyclopedia/grid-time.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.TIME
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Timer ticks
40 REM Action: Returns result of GRID.TIME
50 PRINT GRID.TIME
60 END
```

---

## `GRID.RND(n)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.RND(n)` |
| **Purpose** | Grid random |
| **Action** | Returns result of GRID.RND(n) |
| **Sample** | `programs/encyclopedia/grid-rnd-grid.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.RND(n)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Grid random
40 REM Action: Returns result of GRID.RND(n)
50 PRINT GRID.RND(10)
60 END
```

---

## `GRID.PING(host$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PING(host$)` |
| **Purpose** | Ping host |
| **Action** | Returns result of GRID.PING(host$) |
| **Sample** | `programs/encyclopedia/grid-ping.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PING(host$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Ping host
40 REM Action: Returns result of GRID.PING(host$)
50 PRINT GRID.PING("gateway")
60 END
```

---

## `GRID.SERIAL.READ$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.SERIAL.READ$` |
| **Purpose** | COM1 read line |
| **Action** | Returns result of GRID.SERIAL.READ$ |
| **Sample** | `programs/encyclopedia/grid-serial-read.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.SERIAL.READ$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: COM1 read line
40 REM Action: Returns result of GRID.SERIAL.READ$
50 PRINT LEN(GRID.SERIAL.READ$)
60 END
```

---

## `GRID.STATUS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.STATUS$` |
| **Purpose** | OS status string |
| **Action** | Returns result of GRID.STATUS$ |
| **Sample** | `programs/encyclopedia/grid-status.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.STATUS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: OS status string
40 REM Action: Returns result of GRID.STATUS$
50 PRINT GRID.STATUS$
60 END
```

---

## `GRID.CAP(n)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.CAP(n)` |
| **Purpose** | Capability check |
| **Action** | Returns result of GRID.CAP(n) |
| **Sample** | `programs/encyclopedia/grid-cap.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.CAP(n)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Capability check
40 REM Action: Returns result of GRID.CAP(n)
50 PRINT GRID.CAP(1)
60 END
```

---

## `GRID.INKEY$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.INKEY$` |
| **Purpose** | Poll keyboard |
| **Action** | Returns result of GRID.INKEY$ |
| **Sample** | `programs/encyclopedia/grid-inkey.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.INKEY$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Poll keyboard
40 REM Action: Returns result of GRID.INKEY$
50 PRINT GRID.INKEY$
60 END
```

---

## `GRID.VAULT.GET$(k$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.VAULT.GET$(k$)` |
| **Purpose** | Read vault key |
| **Action** | Returns result of GRID.VAULT.GET$(k$) |
| **Sample** | `programs/encyclopedia/grid-vault-get.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.VAULT.GET$(k$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Read vault key
40 REM Action: Returns result of GRID.VAULT.GET$(k$)
50 PRINT GRID.VAULT.GET$("motd")
60 END
```

---

## `GRID.VAULT.LIST$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.VAULT.LIST$` |
| **Purpose** | List vault keys |
| **Action** | Returns result of GRID.VAULT.LIST$ |
| **Sample** | `programs/encyclopedia/grid-vault-list.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.VAULT.LIST$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: List vault keys
40 REM Action: Returns result of GRID.VAULT.LIST$
50 PRINT GRID.VAULT.LIST$
60 END
```

---

## `GRID.GFS.READ$(path$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.GFS.READ$(path$)` |
| **Purpose** | Read GFS file |
| **Action** | Returns result of GRID.GFS.READ$(path$) |
| **Sample** | `programs/encyclopedia/grid-gfs-read.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.GFS.READ$(path$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Read GFS file
40 REM Action: Returns result of GRID.GFS.READ$(path$)
50 PRINT LEN(GRID.GFS.READ$("/etc/hosts"))
60 END
```

---

## `GRID.GFS.LIST$(path$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.GFS.LIST$(path$)` |
| **Purpose** | List GFS dir |
| **Action** | Returns result of GRID.GFS.LIST$(path$) |
| **Sample** | `programs/encyclopedia/grid-gfs-list.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.GFS.LIST$(path$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: List GFS dir
40 REM Action: Returns result of GRID.GFS.LIST$(path$)
50 PRINT GRID.GFS.LIST$("/programs")
60 END
```

---

## `GRID.HTTP.GET$(h$,port,path$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.HTTP.GET$(h$,port,path$)` |
| **Purpose** | HTTP GET body |
| **Action** | Returns result of GRID.HTTP.GET$(h$,port,path$) |
| **Sample** | `programs/encyclopedia/grid-http-get.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.HTTP.GET$(h$,port,path$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: HTTP GET body
40 REM Action: Returns result of GRID.HTTP.GET$(h$,port,path$)
50 PRINT LEN(GRID.HTTP.GET$("gateway", 80, "/"))
60 END
```

---

## `GRID.HTTP.POST$(...)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.HTTP.POST$(...)` |
| **Purpose** | HTTP POST body |
| **Action** | Returns result of GRID.HTTP.POST$(...) |
| **Sample** | `programs/encyclopedia/grid-http-post.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.HTTP.POST$(...)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: HTTP POST body
40 REM Action: Returns result of GRID.HTTP.POST$(...)
50 PRINT "needs bridge/path"
60 END
```

---

## `GRID.DNS.RESOLVE$(host$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.DNS.RESOLVE$(host$)` |
| **Purpose** | Resolve DNS |
| **Action** | Returns result of GRID.DNS.RESOLVE$(host$) |
| **Sample** | `programs/encyclopedia/grid-dns.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.DNS.RESOLVE$(host$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Resolve DNS
40 REM Action: Returns result of GRID.DNS.RESOLVE$(host$)
50 PRINT GRID.DNS.RESOLVE$("grid")
60 END
```

---

## `GRID.NET.STATUS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.NET.STATUS$` |
| **Purpose** | Network summary |
| **Action** | Returns result of GRID.NET.STATUS$ |
| **Sample** | `programs/encyclopedia/grid-net-status.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.NET.STATUS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Network summary
40 REM Action: Returns result of GRID.NET.STATUS$
50 PRINT GRID.NET.STATUS$
60 END
```

---

## `GRID.LOG.TAIL$(n)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.LOG.TAIL$(n)` |
| **Purpose** | Recent audit lines |
| **Action** | Returns result of GRID.LOG.TAIL$(n) |
| **Sample** | `programs/encyclopedia/grid-log-tail.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.LOG.TAIL$(n)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Recent audit lines
40 REM Action: Returns result of GRID.LOG.TAIL$(n)
50 PRINT GRID.LOG.TAIL$(3)
60 END
```

---

## `GRID.WHOAMI$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.WHOAMI$` |
| **Purpose** | Entity type |
| **Action** | Returns result of GRID.WHOAMI$ |
| **Sample** | `programs/encyclopedia/grid-whoami.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.WHOAMI$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Entity type
40 REM Action: Returns result of GRID.WHOAMI$
50 PRINT GRID.WHOAMI$
60 END
```

---

## `GRID.CAPS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.CAPS$` |
| **Purpose** | Capability mask |
| **Action** | Returns result of GRID.CAPS$ |
| **Sample** | `programs/encyclopedia/grid-caps.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.CAPS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Capability mask
40 REM Action: Returns result of GRID.CAPS$
50 PRINT GRID.CAPS$
60 END
```

---

## `GRID.JOBS.LIST$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.JOBS.LIST$` |
| **Purpose** | Background jobs |
| **Action** | Returns result of GRID.JOBS.LIST$ |
| **Sample** | `programs/encyclopedia/grid-jobs-list.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.JOBS.LIST$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Background jobs
40 REM Action: Returns result of GRID.JOBS.LIST$
50 PRINT GRID.JOBS.LIST$
60 END
```

---

## `GRID.ISO.LIST$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.ISO.LIST$` |
| **Purpose** | ISO entities |
| **Action** | Returns result of GRID.ISO.LIST$ |
| **Sample** | `programs/encyclopedia/grid-iso-list.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.ISO.LIST$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: ISO entities
40 REM Action: Returns result of GRID.ISO.LIST$
50 PRINT GRID.ISO.LIST$
60 END
```

---

## `GRID.DISC.STATUS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.DISC.STATUS$` |
| **Purpose** | Disc summary |
| **Action** | Returns result of GRID.DISC.STATUS$ |
| **Sample** | `programs/encyclopedia/grid-disc-status.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.DISC.STATUS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Disc summary
40 REM Action: Returns result of GRID.DISC.STATUS$
50 PRINT GRID.DISC.STATUS$
60 END
```

---

## `GRID.DISC.ENTITY$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.DISC.ENTITY$` |
| **Purpose** | Disc entity |
| **Action** | Returns result of GRID.DISC.ENTITY$ |
| **Sample** | `programs/encyclopedia/grid-disc-entity.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.DISC.ENTITY$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Disc entity
40 REM Action: Returns result of GRID.DISC.ENTITY$
50 PRINT GRID.DISC.ENTITY$
60 END
```

---

## `GRID.DISC.LEVEL`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.DISC.LEVEL` |
| **Purpose** | Disc level |
| **Action** | Returns result of GRID.DISC.LEVEL |
| **Sample** | `programs/encyclopedia/grid-disc-level.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.DISC.LEVEL
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Disc level
40 REM Action: Returns result of GRID.DISC.LEVEL
50 PRINT GRID.DISC.LEVEL
60 END
```

---

## `GRID.DISC.XP`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.DISC.XP` |
| **Purpose** | Disc XP |
| **Action** | Returns result of GRID.DISC.XP |
| **Sample** | `programs/encyclopedia/grid-disc-xp.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.DISC.XP
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Disc XP
40 REM Action: Returns result of GRID.DISC.XP
50 PRINT GRID.DISC.XP
60 END
```

---

## `GRID.RECOGNIZER.STATUS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.RECOGNIZER.STATUS$` |
| **Purpose** | Patrol status |
| **Action** | Returns result of GRID.RECOGNIZER.STATUS$ |
| **Sample** | `programs/encyclopedia/grid-recog-status.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.RECOGNIZER.STATUS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Patrol status
40 REM Action: Returns result of GRID.RECOGNIZER.STATUS$
50 PRINT GRID.RECOGNIZER.STATUS$
60 END
```

---

## `GRID.PKG.LIST$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PKG.LIST$` |
| **Purpose** | Package list |
| **Action** | Returns result of GRID.PKG.LIST$ |
| **Sample** | `programs/encyclopedia/grid-pkg-list.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PKG.LIST$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Package list
40 REM Action: Returns result of GRID.PKG.LIST$
50 PRINT GRID.PKG.LIST$
60 END
```

---

## `GRID.PKG.MODS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.PKG.MODS$` |
| **Purpose** | Module list |
| **Action** | Returns result of GRID.PKG.MODS$ |
| **Sample** | `programs/encyclopedia/grid-pkg-mods.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.PKG.MODS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Module list
40 REM Action: Returns result of GRID.PKG.MODS$
50 PRINT GRID.PKG.MODS$
60 END
```

---

## `GRID.AI.ASK$(prompt$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.AI.ASK$(prompt$)` |
| **Purpose** | Ask AI |
| **Action** | Returns result of GRID.AI.ASK$(prompt$) |
| **Sample** | `programs/encyclopedia/grid-ai-ask.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.AI.ASK$(prompt$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Ask AI
40 REM Action: Returns result of GRID.AI.ASK$(prompt$)
50 PRINT GRID.AI.ASK$("What is DIM?", "ASK")
60 END
```

---

## `GRID.AI.COMPLETE$(frag$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.AI.COMPLETE$(frag$)` |
| **Purpose** | AI complete |
| **Action** | Returns result of GRID.AI.COMPLETE$(frag$) |
| **Sample** | `programs/encyclopedia/grid-ai-complete.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.AI.COMPLETE$(frag$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: AI complete
40 REM Action: Returns result of GRID.AI.COMPLETE$(frag$)
50 PRINT LEN(GRID.AI.COMPLETE$("PRINT "))
60 END
```

---

## `GRID.AI.EXPLAIN$(line$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.AI.EXPLAIN$(line$)` |
| **Purpose** | AI explain |
| **Action** | Returns result of GRID.AI.EXPLAIN$(line$) |
| **Sample** | `programs/encyclopedia/grid-ai-explain.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.AI.EXPLAIN$(line$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: AI explain
40 REM Action: Returns result of GRID.AI.EXPLAIN$(line$)
50 PRINT GRID.AI.EXPLAIN$("PRINT 1")
60 END
```

---

## `GRID.AI.FIX$(code$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.AI.FIX$(code$)` |
| **Purpose** | AI fix code |
| **Action** | Returns result of GRID.AI.FIX$(code$) |
| **Sample** | `programs/encyclopedia/grid-ai-fix.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.AI.FIX$(code$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: AI fix code
40 REM Action: Returns result of GRID.AI.FIX$(code$)
50 PRINT LEN(GRID.AI.FIX$("PRNT 1"))
60 END
```

---

## `GRID.AI.MODELS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.AI.MODELS$` |
| **Purpose** | AI models info |
| **Action** | Returns result of GRID.AI.MODELS$ |
| **Sample** | `programs/encyclopedia/grid-ai-models.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.AI.MODELS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: AI models info
40 REM Action: Returns result of GRID.AI.MODELS$
50 PRINT GRID.AI.MODELS$
60 END
```

---

## `GRID.IRC.READ$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.IRC.READ$` |
| **Purpose** | IRC read line |
| **Action** | Returns result of GRID.IRC.READ$ |
| **Sample** | `programs/encyclopedia/grid-irc-read.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.IRC.READ$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: IRC read line
40 REM Action: Returns result of GRID.IRC.READ$
50 PRINT GRID.IRC.READ$
60 END
```

---

## `GRID.IRC.STATUS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.IRC.STATUS$` |
| **Purpose** | IRC status |
| **Action** | Returns result of GRID.IRC.STATUS$ |
| **Sample** | `programs/encyclopedia/grid-irc-status.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.IRC.STATUS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: IRC status
40 REM Action: Returns result of GRID.IRC.STATUS$
50 PRINT GRID.IRC.STATUS$
60 END
```

---

## `GRID.IRC.CONNECT$(h$,p,n$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.IRC.CONNECT$(h$,p,n$)` |
| **Purpose** | IRC connect result |
| **Action** | Returns result of GRID.IRC.CONNECT$(h$,p,n$) |
| **Sample** | `programs/encyclopedia/grid-irc-connect-fn.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.IRC.CONNECT$(h$,p,n$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: IRC connect result
40 REM Action: Returns result of GRID.IRC.CONNECT$(h$,p,n$)
50 PRINT GRID.IRC.CONNECT$("gateway", 6667, "u")
60 END
```

---

## `GRID.BTC.CALL$(m$, params$)`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.CALL$(m$, params$)` |
| **Purpose** | BTC RPC call |
| **Action** | Returns result of GRID.BTC.CALL$(m$, params$) |
| **Sample** | `programs/encyclopedia/grid-btc-call.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.CALL$(m$, params$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: BTC RPC call
40 REM Action: Returns result of GRID.BTC.CALL$(m$, params$)
50 PRINT GRID.BTC.CALL$("getnetworkinfo", "")
60 END
```

---

## `GRID.BTC.INFO$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.INFO$` |
| **Purpose** | Blockchain info |
| **Action** | Returns result of GRID.BTC.INFO$ |
| **Sample** | `programs/encyclopedia/grid-btc-info.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.INFO$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Blockchain info
40 REM Action: Returns result of GRID.BTC.INFO$
50 PRINT GRID.BTC.INFO$
60 END
```

---

## `GRID.BTC.NETWORK$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.NETWORK$` |
| **Purpose** | Network info |
| **Action** | Returns result of GRID.BTC.NETWORK$ |
| **Sample** | `programs/encyclopedia/grid-btc-network.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.NETWORK$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Network info
40 REM Action: Returns result of GRID.BTC.NETWORK$
50 PRINT GRID.BTC.NETWORK$
60 END
```

---

## `GRID.BTC.WALLET$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.WALLET$` |
| **Purpose** | Wallet info |
| **Action** | Returns result of GRID.BTC.WALLET$ |
| **Sample** | `programs/encyclopedia/grid-btc-wallet.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.WALLET$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Wallet info
40 REM Action: Returns result of GRID.BTC.WALLET$
50 PRINT GRID.BTC.WALLET$
60 END
```

---

## `GRID.BTC.BALANCE$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.BALANCE$` |
| **Purpose** | Wallet balance |
| **Action** | Returns result of GRID.BTC.BALANCE$ |
| **Sample** | `programs/encyclopedia/grid-btc-balance.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.BALANCE$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Wallet balance
40 REM Action: Returns result of GRID.BTC.BALANCE$
50 PRINT GRID.BTC.BALANCE$
60 END
```

---

## `GRID.BTC.ADDRESS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.ADDRESS$` |
| **Purpose** | New address |
| **Action** | Returns result of GRID.BTC.ADDRESS$ |
| **Sample** | `programs/encyclopedia/grid-btc-address.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.ADDRESS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: New address
40 REM Action: Returns result of GRID.BTC.ADDRESS$
50 PRINT GRID.BTC.ADDRESS$
60 END
```

---

## `GRID.BTC.HELP$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.HELP$` |
| **Purpose** | BTC help text |
| **Action** | Returns result of GRID.BTC.HELP$ |
| **Sample** | `programs/encyclopedia/grid-btc-help.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.HELP$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: BTC help text
40 REM Action: Returns result of GRID.BTC.HELP$
50 PRINT GRID.BTC.HELP$
60 END
```

---

## `GRID.BTC.STATUS$`

| Field | Value |
|-------|-------|
| **Where** | GridBASIC program (GRID binding) |
| **Syntax** | `GRID.BTC.STATUS$` |
| **Purpose** | Bridge status |
| **Action** | Returns result of GRID.BTC.STATUS$ |
| **Sample** | `programs/encyclopedia/grid-btc-status.bas` |
| **See also** | [grid-bindings.md](../grid-bindings.md) |

```basic
10 REM Encyclopedia: GRID.BTC.STATUS$
20 REM Where: GridBASIC GRID binding
30 REM Purpose: Bridge status
40 REM Action: Returns result of GRID.BTC.STATUS$
50 PRINT GRID.BTC.STATUS$
60 END
```

---
