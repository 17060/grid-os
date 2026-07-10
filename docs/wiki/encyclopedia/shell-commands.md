# 03 — Flynn shell commands

105 encyclopedia entries.

## `help`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `help` |
| **Purpose** | Show shell command list |
| **Action** | Prints all grid> commands |
| **Sample** | `programs/encyclopedia/cmd-help.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: help
20 REM Where: grid> shell
30 REM Purpose: Show shell command list
40 REM Action: Prints all grid> commands
50 PRINT "At grid> type: help"
60 END
```

---

## `disc`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `disc` |
| **Purpose** | Show identity disc |
| **Action** | Displays disc level, XP, entity |
| **Sample** | `programs/encyclopedia/cmd-disc.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: disc
20 REM Where: grid> shell
30 REM Purpose: Show identity disc
40 REM Action: Displays disc level, XP, entity
50 PRINT "At grid> type: disc"
60 END
```

---

## `whoami`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `whoami` |
| **Purpose** | Show entity type |
| **Action** | Prints User or Program |
| **Sample** | `programs/encyclopedia/cmd-whoami.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: whoami
20 REM Where: grid> shell
30 REM Purpose: Show entity type
40 REM Action: Prints User or Program
50 PRINT "At grid> type: whoami"
60 END
```

---

## `caps`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `caps` |
| **Purpose** | Show capabilities |
| **Action** | Decodes granted cap bitmask |
| **Sample** | `programs/encyclopedia/cmd-caps.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: caps
20 REM Where: grid> shell
30 REM Purpose: Show capabilities
40 REM Action: Decodes granted cap bitmask
50 PRINT "At grid> type: caps"
60 END
```

---

## `status`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `status` |
| **Purpose** | Runtime status |
| **Action** | Prints Grid OS status string |
| **Sample** | `programs/encyclopedia/cmd-status.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: status
20 REM Where: grid> shell
30 REM Purpose: Runtime status
40 REM Action: Prints Grid OS status string
50 PRINT "At grid> type: status"
60 END
```

---

## `cycles`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `cycles` |
| **Purpose** | Elapsed cycles |
| **Action** | Same summary as status |
| **Sample** | `programs/encyclopedia/cmd-cycles.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: cycles
20 REM Where: grid> shell
30 REM Purpose: Elapsed cycles
40 REM Action: Same summary as status
50 PRINT "At grid> type: cycles"
60 END
```

---

## `vision`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vision` |
| **Purpose** | Flynn principles |
| **Action** | Prints founding vision text |
| **Sample** | `programs/encyclopedia/cmd-vision.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vision
20 REM Where: grid> shell
30 REM Purpose: Flynn principles
40 REM Action: Prints founding vision text
50 PRINT "At grid> type: vision"
60 END
```

---

## `clear`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `clear` |
| **Purpose** | Clear screen |
| **Action** | Clears console and redraws banner |
| **Sample** | `programs/encyclopedia/cmd-clear.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: clear
20 REM Where: grid> shell
30 REM Purpose: Clear screen
40 REM Action: Clears console and redraws banner
50 PRINT "At grid> type: clear"
60 END
```

---

## `about`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `about` |
| **Purpose** | About Grid OS |
| **Action** | Version and credits |
| **Sample** | `programs/encyclopedia/cmd-about.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: about
20 REM Where: grid> shell
30 REM Purpose: About Grid OS
40 REM Action: Version and credits
50 PRINT "At grid> type: about"
60 END
```

---

## `poweroff`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `poweroff` |
| **Purpose** | Exit Grid OS |
| **Action** | Triggers isa-debug-exit in QEMU |
| **Sample** | `programs/encyclopedia/cmd-poweroff.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: poweroff
20 REM Where: grid> shell
30 REM Purpose: Exit Grid OS
40 REM Action: Triggers isa-debug-exit in QEMU
50 PRINT "At grid> type: poweroff"
60 END
```

---

## `spawn [name]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `spawn [name]` |
| **Purpose** | Run ring-3 program |
| **Action** | Loads ELF sandbox (default gridsh) |
| **Sample** | `programs/encyclopedia/cmd-spawn.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: spawn [name]
20 REM Where: grid> shell
30 REM Purpose: Run ring-3 program
40 REM Action: Loads ELF sandbox (default gridsh)
50 PRINT "At grid> type: spawn [name]"
60 END
```

---

## `spawn bg <name>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `spawn bg <name>` |
| **Purpose** | Background spawn |
| **Action** | Queues sandbox job |
| **Sample** | `programs/encyclopedia/cmd-spawn-bg.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: spawn bg <name>
20 REM Where: grid> shell
30 REM Purpose: Background spawn
40 REM Action: Queues sandbox job
50 PRINT "At grid> type: spawn bg <name>"
60 END
```

---

## `spawn list`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `spawn list` |
| **Purpose** | List spawnable programs |
| **Action** | Shows /programs/*.elf catalog |
| **Sample** | `programs/encyclopedia/cmd-spawn-list.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: spawn list
20 REM Where: grid> shell
30 REM Purpose: List spawnable programs
40 REM Action: Shows /programs/*.elf catalog
50 PRINT "At grid> type: spawn list"
60 END
```

---

## `catalog`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `catalog` |
| **Purpose** | Spawn catalog |
| **Action** | Lists ring-3 programs on disk |
| **Sample** | `programs/encyclopedia/cmd-catalog.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: catalog
20 REM Where: grid> shell
30 REM Purpose: Spawn catalog
40 REM Action: Lists ring-3 programs on disk
50 PRINT "At grid> type: catalog"
60 END
```

---

## `programs`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `programs` |
| **Purpose** | Spawn history |
| **Action** | Shows previously spawned programs |
| **Sample** | `programs/encyclopedia/cmd-programs.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: programs
20 REM Where: grid> shell
30 REM Purpose: Spawn history
40 REM Action: Shows previously spawned programs
50 PRINT "At grid> type: programs"
60 END
```

---

## `jobs`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `jobs` |
| **Purpose** | Background jobs |
| **Action** | Lists active sandbox jobs |
| **Sample** | `programs/encyclopedia/cmd-jobs.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: jobs
20 REM Where: grid> shell
30 REM Purpose: Background jobs
40 REM Action: Lists active sandbox jobs
50 PRINT "At grid> type: jobs"
60 END
```

---

## `kill <#>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `kill <#>` |
| **Purpose** | Stop job |
| **Action** | Terminates background job by number |
| **Sample** | `programs/encyclopedia/cmd-kill.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: kill <#>
20 REM Where: grid> shell
30 REM Purpose: Stop job
40 REM Action: Terminates background job by number
50 PRINT "At grid> type: kill <#>"
60 END
```

---

## `kill all`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `kill all` |
| **Purpose** | Stop all jobs |
| **Action** | Terminates every background job |
| **Sample** | `programs/encyclopedia/cmd-kill-all.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: kill all
20 REM Where: grid> shell
30 REM Purpose: Stop all jobs
40 REM Action: Terminates every background job
50 PRINT "At grid> type: kill all"
60 END
```

---

## `fg <#>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `fg <#>` |
| **Purpose** | Foreground job |
| **Action** | Runs background job in foreground |
| **Sample** | `programs/encyclopedia/cmd-fg.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: fg <#>
20 REM Where: grid> shell
30 REM Purpose: Foreground job
40 REM Action: Runs background job in foreground
50 PRINT "At grid> type: fg <#>"
60 END
```

---

## `wait`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `wait` |
| **Purpose** | Wait for jobs |
| **Action** | Blocks until all bg jobs finish |
| **Sample** | `programs/encyclopedia/cmd-wait.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: wait
20 REM Where: grid> shell
30 REM Purpose: Wait for jobs
40 REM Action: Blocks until all bg jobs finish
50 PRINT "At grid> type: wait"
60 END
```

---

## `echo <text>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `echo <text>` |
| **Purpose** | Print text |
| **Action** | Writes argument to console |
| **Sample** | `programs/encyclopedia/cmd-echo.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: echo <text>
20 REM Where: grid> shell
30 REM Purpose: Print text
40 REM Action: Writes argument to console
50 PRINT "At grid> type: echo <text>"
60 END
```

---

## `ls [path]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ls [path]` |
| **Purpose** | List GridFS |
| **Action** | Directory listing, default / |
| **Sample** | `programs/encyclopedia/cmd-ls.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ls [path]
20 REM Where: grid> shell
30 REM Purpose: List GridFS
40 REM Action: Directory listing, default /
50 PRINT "At grid> type: ls [path]"
60 END
```

---

## `cat <path>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `cat <path>` |
| **Purpose** | Read GridFS file |
| **Action** | Prints file contents |
| **Sample** | `programs/encyclopedia/cmd-cat.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: cat <path>
20 REM Where: grid> shell
30 REM Purpose: Read GridFS file
40 REM Action: Prints file contents
50 PRINT "At grid> type: cat <path>"
60 END
```

---

## `gfs`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `gfs` |
| **Purpose** | Flynn archive status |
| **Action** | Shows GFS mount info |
| **Sample** | `programs/encyclopedia/cmd-gfs.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: gfs
20 REM Where: grid> shell
30 REM Purpose: Flynn archive status
40 REM Action: Shows GFS mount info
50 PRINT "At grid> type: gfs"
60 END
```

---

## `gfs list`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `gfs list` |
| **Purpose** | List GFS root |
| **Action** | Lists top-level Flynn paths |
| **Sample** | `programs/encyclopedia/cmd-gfs-list.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: gfs list
20 REM Where: grid> shell
30 REM Purpose: List GFS root
40 REM Action: Lists top-level Flynn paths
50 PRINT "At grid> type: gfs list"
60 END
```

---

## `gfs seed`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `gfs seed` |
| **Purpose** | Re-seed disk files |
| **Action** | Restores default GFS entries |
| **Sample** | `programs/encyclopedia/cmd-gfs-seed.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: gfs seed
20 REM Where: grid> shell
30 REM Purpose: Re-seed disk files
40 REM Action: Restores default GFS entries
50 PRINT "At grid> type: gfs seed"
60 END
```

---

## `log`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `log` |
| **Purpose** | Full audit log |
| **Action** | Prints entire audit trail |
| **Sample** | `programs/encyclopedia/cmd-log.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: log
20 REM Where: grid> shell
30 REM Purpose: Full audit log
40 REM Action: Prints entire audit trail
50 PRINT "At grid> type: log"
60 END
```

---

## `log tail`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `log tail` |
| **Purpose** | Recent audit entries |
| **Action** | Last 10 log lines |
| **Sample** | `programs/encyclopedia/cmd-log-tail.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: log tail
20 REM Where: grid> shell
30 REM Purpose: Recent audit entries
40 REM Action: Last 10 log lines
50 PRINT "At grid> type: log tail"
60 END
```

---

## `vault list`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vault list` |
| **Purpose** | List vault keys |
| **Action** | Shows persistent key/value nodes |
| **Sample** | `programs/encyclopedia/cmd-vault-list.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vault list
20 REM Where: grid> shell
30 REM Purpose: List vault keys
40 REM Action: Shows persistent key/value nodes
50 PRINT "At grid> type: vault list"
60 END
```

---

## `vault put <k> <v>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vault put <k> <v>` |
| **Purpose** | Store vault node |
| **Action** | Sets key to value in vault |
| **Sample** | `programs/encyclopedia/cmd-vault-put.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vault put <k> <v>
20 REM Where: grid> shell
30 REM Purpose: Store vault node
40 REM Action: Sets key to value in vault
50 PRINT "At grid> type: vault put <k> <v>"
60 END
```

---

## `vault get <k>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vault get <k>` |
| **Purpose** | Read vault node |
| **Action** | Prints value for key |
| **Sample** | `programs/encyclopedia/cmd-vault-get.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vault get <k>
20 REM Where: grid> shell
30 REM Purpose: Read vault node
40 REM Action: Prints value for key
50 PRINT "At grid> type: vault get <k>"
60 END
```

---

## `vault save`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vault save` |
| **Purpose** | Vault memory snapshot |
| **Action** | CRC-seals in-memory vault |
| **Sample** | `programs/encyclopedia/cmd-vault-save.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vault save
20 REM Where: grid> shell
30 REM Purpose: Vault memory snapshot
40 REM Action: CRC-seals in-memory vault
50 PRINT "At grid> type: vault save"
60 END
```

---

## `vault sync`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vault sync` |
| **Purpose** | Persist vault |
| **Action** | Writes vault to arcade disk |
| **Sample** | `programs/encyclopedia/cmd-vault-sync.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vault sync
20 REM Where: grid> shell
30 REM Purpose: Persist vault
40 REM Action: Writes vault to arcade disk
50 PRINT "At grid> type: vault sync"
60 END
```

---

## `vault export`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vault export` |
| **Purpose** | Export vault COM1 |
| **Action** | Serial export frame |
| **Sample** | `programs/encyclopedia/cmd-vault-export.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vault export
20 REM Where: grid> shell
30 REM Purpose: Export vault COM1
40 REM Action: Serial export frame
50 PRINT "At grid> type: vault export"
60 END
```

---

## `vault import`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `vault import` |
| **Purpose** | Import vault COM1 |
| **Action** | Serial import frame |
| **Sample** | `programs/encyclopedia/cmd-vault-import.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: vault import
20 REM Where: grid> shell
30 REM Purpose: Import vault COM1
40 REM Action: Serial import frame
50 PRINT "At grid> type: vault import"
60 END
```

---

## `net status`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `net status` |
| **Purpose** | Network status |
| **Action** | virtio-net and IP summary |
| **Sample** | `programs/encyclopedia/cmd-net.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: net status
20 REM Where: grid> shell
30 REM Purpose: Network status
40 REM Action: virtio-net and IP summary
50 PRINT "At grid> type: net status"
60 END
```

---

## `net ping <host>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `net ping <host>` |
| **Purpose** | ICMP ping |
| **Action** | Pings host or built-in name |
| **Sample** | `programs/encyclopedia/cmd-net-ping.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: net ping <host>
20 REM Where: grid> shell
30 REM Purpose: ICMP ping
40 REM Action: Pings host or built-in name
50 PRINT "At grid> type: net ping <host>"
60 END
```

---

## `net poll`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `net poll` |
| **Purpose** | Drain net queue |
| **Action** | Processes pending packets |
| **Sample** | `programs/encyclopedia/cmd-net-poll.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: net poll
20 REM Where: grid> shell
30 REM Purpose: Drain net queue
40 REM Action: Processes pending packets
50 PRINT "At grid> type: net poll"
60 END
```

---

## `http get <host> [port] <path>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `http get <host> [port] <path>` |
| **Purpose** | HTTP GET |
| **Action** | HTTP/1.1 GET with keep-alive |
| **Sample** | `programs/encyclopedia/cmd-http-get.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: http get <host> [port] <path>
20 REM Where: grid> shell
30 REM Purpose: HTTP GET
40 REM Action: HTTP/1.1 GET with keep-alive
50 PRINT "At grid> type: http get <host> [port] <path>"
60 END
```

---

## `http post <host> [port] <path> <body>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `http post <host> [port] <path> <body>` |
| **Purpose** | HTTP POST |
| **Action** | HTTP/1.1 POST request |
| **Sample** | `programs/encyclopedia/cmd-http-post.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: http post <host> [port] <path> <body>
20 REM Where: grid> shell
30 REM Purpose: HTTP POST
40 REM Action: HTTP/1.1 POST request
50 PRINT "At grid> type: http post <host> [port] <path> <body>"
60 END
```

---

## `irc connect <h> <p> <nick>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `irc connect <h> <p> <nick>` |
| **Purpose** | IRC connect |
| **Action** | Opens persistent IRC TCP session |
| **Sample** | `programs/encyclopedia/cmd-irc-connect.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: irc connect <h> <p> <nick>
20 REM Where: grid> shell
30 REM Purpose: IRC connect
40 REM Action: Opens persistent IRC TCP session
50 PRINT "At grid> type: irc connect <h> <p> <nick>"
60 END
```

---

## `irc join <#chan>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `irc join <#chan>` |
| **Purpose** | IRC join |
| **Action** | JOIN channel on active session |
| **Sample** | `programs/encyclopedia/cmd-irc-join.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: irc join <#chan>
20 REM Where: grid> shell
30 REM Purpose: IRC join
40 REM Action: JOIN channel on active session
50 PRINT "At grid> type: irc join <#chan>"
60 END
```

---

## `irc say <#c> <msg>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `irc say <#c> <msg>` |
| **Purpose** | IRC message |
| **Action** | Sends PRIVMSG |
| **Sample** | `programs/encyclopedia/cmd-irc-say.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: irc say <#c> <msg>
20 REM Where: grid> shell
30 REM Purpose: IRC message
40 REM Action: Sends PRIVMSG
50 PRINT "At grid> type: irc say <#c> <msg>"
60 END
```

---

## `irc read`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `irc read` |
| **Purpose** | IRC read queue |
| **Action** | Prints queued server lines |
| **Sample** | `programs/encyclopedia/cmd-irc-read.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: irc read
20 REM Where: grid> shell
30 REM Purpose: IRC read queue
40 REM Action: Prints queued server lines
50 PRINT "At grid> type: irc read"
60 END
```

---

## `irc status`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `irc status` |
| **Purpose** | IRC session info |
| **Action** | Connection summary |
| **Sample** | `programs/encyclopedia/cmd-irc-status.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: irc status
20 REM Where: grid> shell
30 REM Purpose: IRC session info
40 REM Action: Connection summary
50 PRINT "At grid> type: irc status"
60 END
```

---

## `irc quit`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `irc quit` |
| **Purpose** | IRC disconnect |
| **Action** | Sends QUIT and closes |
| **Sample** | `programs/encyclopedia/cmd-irc-quit.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: irc quit
20 REM Where: grid> shell
30 REM Purpose: IRC disconnect
40 REM Action: Sends QUIT and closes
50 PRINT "At grid> type: irc quit"
60 END
```

---

## `portal`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `portal` |
| **Purpose** | GridLink portal |
| **Action** | Portal connection status |
| **Sample** | `programs/encyclopedia/cmd-portal.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: portal
20 REM Where: grid> shell
30 REM Purpose: GridLink portal
40 REM Action: Portal connection status
50 PRINT "At grid> type: portal"
60 END
```

---

## `portal export`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `portal export` |
| **Purpose** | Export vault link |
| **Action** | COM1 vault frame |
| **Sample** | `programs/encyclopedia/cmd-portal-export.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: portal export
20 REM Where: grid> shell
30 REM Purpose: Export vault link
40 REM Action: COM1 vault frame
50 PRINT "At grid> type: portal export"
60 END
```

---

## `portal import`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `portal import` |
| **Purpose** | Import vault link |
| **Action** | Receive vault from host |
| **Sample** | `programs/encyclopedia/cmd-portal-import.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: portal import
20 REM Where: grid> shell
30 REM Purpose: Import vault link
40 REM Action: Receive vault from host
50 PRINT "At grid> type: portal import"
60 END
```

---

## `portal recv`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `portal recv` |
| **Purpose** | Receive program |
| **Action** | Install /programs/* via GridLink |
| **Sample** | `programs/encyclopedia/cmd-portal-recv.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: portal recv
20 REM Where: grid> shell
30 REM Purpose: Receive program
40 REM Action: Install /programs/* via GridLink
50 PRINT "At grid> type: portal recv"
60 END
```

---

## `portal pkg`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `portal pkg` |
| **Purpose** | Receive package |
| **Action** | Install .gridpkg bundle |
| **Sample** | `programs/encyclopedia/cmd-portal-pkg.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: portal pkg
20 REM Where: grid> shell
30 REM Purpose: Receive package
40 REM Action: Install .gridpkg bundle
50 PRINT "At grid> type: portal pkg"
60 END
```

---

## `pkg list`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `pkg list` |
| **Purpose** | List packages |
| **Action** | Installed Grid packages |
| **Sample** | `programs/encyclopedia/cmd-pkg-list.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: pkg list
20 REM Where: grid> shell
30 REM Purpose: List packages
40 REM Action: Installed Grid packages
50 PRINT "At grid> type: pkg list"
60 END
```

---

## `pkg mods [cat]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `pkg mods [cat]` |
| **Purpose** | List IDE modules |
| **Action** | Module names, optional category |
| **Sample** | `programs/encyclopedia/cmd-pkg-mods.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: pkg mods [cat]
20 REM Where: grid> shell
30 REM Purpose: List IDE modules
40 REM Action: Module names, optional category
50 PRINT "At grid> type: pkg mods [cat]"
60 END
```

---

## `pkg info <name>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `pkg info <name>` |
| **Purpose** | Package details |
| **Action** | Version, desc, module list |
| **Sample** | `programs/encyclopedia/cmd-pkg-info.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: pkg info <name>
20 REM Where: grid> shell
30 REM Purpose: Package details
40 REM Action: Version, desc, module list
50 PRINT "At grid> type: pkg info <name>"
60 END
```

---

## `pkg install <manifest>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `pkg install <manifest>` |
| **Purpose** | Install package |
| **Action** | Register MANIFEST on disk |
| **Sample** | `programs/encyclopedia/cmd-pkg-install.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: pkg install <manifest>
20 REM Where: grid> shell
30 REM Purpose: Install package
40 REM Action: Register MANIFEST on disk
50 PRINT "At grid> type: pkg install <manifest>"
60 END
```

---

## `pkg remove <name>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `pkg remove <name>` |
| **Purpose** | Remove package |
| **Action** | Uninstall package files |
| **Sample** | `programs/encyclopedia/cmd-pkg-remove.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: pkg remove <name>
20 REM Where: grid> shell
30 REM Purpose: Remove package
40 REM Action: Uninstall package files
50 PRINT "At grid> type: pkg remove <name>"
60 END
```

---

## `pkg recv`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `pkg recv` |
| **Purpose** | Receive PKG COM1 |
| **Action** | GridLink package frame |
| **Sample** | `programs/encyclopedia/cmd-pkg-recv.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: pkg recv
20 REM Where: grid> shell
30 REM Purpose: Receive PKG COM1
40 REM Action: GridLink package frame
50 PRINT "At grid> type: pkg recv"
60 END
```

---

## `serial status`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `serial status` |
| **Purpose** | COM1 status |
| **Action** | Online/offline at 0x3F8 |
| **Sample** | `programs/encyclopedia/cmd-serial-status.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: serial status
20 REM Where: grid> shell
30 REM Purpose: COM1 status
40 REM Action: Online/offline at 0x3F8
50 PRINT "At grid> type: serial status"
60 END
```

---

## `serial write <text>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `serial write <text>` |
| **Purpose** | COM1 transmit |
| **Action** | Writes text to serial port |
| **Sample** | `programs/encyclopedia/cmd-serial-write.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: serial write <text>
20 REM Where: grid> shell
30 REM Purpose: COM1 transmit
40 REM Action: Writes text to serial port
50 PRINT "At grid> type: serial write <text>"
60 END
```

---

## `serial read`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `serial read` |
| **Purpose** | COM1 read line |
| **Action** | Reads one line from COM1 |
| **Sample** | `programs/encyclopedia/cmd-serial-read.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: serial read
20 REM Where: grid> shell
30 REM Purpose: COM1 read line
40 REM Action: Reads one line from COM1
50 PRINT "At grid> type: serial read"
60 END
```

---

## `iso zone`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso zone` |
| **Purpose** | ISO research zone |
| **Action** | Zone status summary |
| **Sample** | `programs/encyclopedia/cmd-iso-zone.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso zone
20 REM Where: grid> shell
30 REM Purpose: ISO research zone
40 REM Action: Zone status summary
50 PRINT "At grid> type: iso zone"
60 END
```

---

## `iso list`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso list` |
| **Purpose** | List ISO entities |
| **Action** | Research zone entity list |
| **Sample** | `programs/encyclopedia/cmd-iso-list.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso list
20 REM Where: grid> shell
30 REM Purpose: List ISO entities
40 REM Action: Research zone entity list
50 PRINT "At grid> type: iso list"
60 END
```

---

## `iso spawn [name]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso spawn [name]` |
| **Purpose** | Spawn ISO |
| **Action** | Seeds new ISO research entity |
| **Sample** | `programs/encyclopedia/cmd-iso-spawn.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso spawn [name]
20 REM Where: grid> shell
30 REM Purpose: Spawn ISO
40 REM Action: Seeds new ISO research entity
50 PRINT "At grid> type: iso spawn [name]"
60 END
```

---

## `iso inspect <id>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso inspect <id>` |
| **Purpose** | Inspect ISO |
| **Action** | Genome and disc details |
| **Sample** | `programs/encyclopedia/cmd-iso-inspect.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso inspect <id>
20 REM Where: grid> shell
30 REM Purpose: Inspect ISO
40 REM Action: Genome and disc details
50 PRINT "At grid> type: iso inspect <id>"
60 END
```

---

## `iso evolve <id>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso evolve <id>` |
| **Purpose** | Evolve ISO |
| **Action** | Mutate genome in sandbox |
| **Sample** | `programs/encyclopedia/cmd-iso-evolve.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso evolve <id>
20 REM Where: grid> shell
30 REM Purpose: Evolve ISO
40 REM Action: Mutate genome in sandbox
50 PRINT "At grid> type: iso evolve <id>"
60 END
```

---

## `iso quarantine <id>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso quarantine <id>` |
| **Purpose** | Quarantine ISO |
| **Action** | Isolates anomaly |
| **Sample** | `programs/encyclopedia/cmd-iso-quarantine.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso quarantine <id>
20 REM Where: grid> shell
30 REM Purpose: Quarantine ISO
40 REM Action: Isolates anomaly
50 PRINT "At grid> type: iso quarantine <id>"
60 END
```

---

## `iso release <id>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso release <id>` |
| **Purpose** | Release ISO |
| **Action** | Restores quarantined entity |
| **Sample** | `programs/encyclopedia/cmd-iso-release.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso release <id>
20 REM Where: grid> shell
30 REM Purpose: Release ISO
40 REM Action: Restores quarantined entity
50 PRINT "At grid> type: iso release <id>"
60 END
```

---

## `iso autopilot on|off`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `iso autopilot on|off` |
| **Purpose** | ISO autopilot |
| **Action** | Background evolution toggle |
| **Sample** | `programs/encyclopedia/cmd-iso-autopilot.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: iso autopilot on|off
20 REM Where: grid> shell
30 REM Purpose: ISO autopilot
40 REM Action: Background evolution toggle
50 PRINT "At grid> type: iso autopilot on|off"
60 END
```

---

## `basic`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `basic` |
| **Purpose** | Open IDE |
| **Action** | Enters GridBASIC IDE |
| **Sample** | `programs/encyclopedia/cmd-basic.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: basic
20 REM Where: grid> shell
30 REM Purpose: Open IDE
40 REM Action: Enters GridBASIC IDE
50 PRINT "At grid> type: basic"
60 END
```

---

## `basic run <file>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `basic run <file>` |
| **Purpose** | Run GFS program |
| **Action** | Executes .bas or .grid from disk |
| **Sample** | `programs/encyclopedia/cmd-basic-run.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: basic run <file>
20 REM Where: grid> shell
30 REM Purpose: Run GFS program
40 REM Action: Executes .bas or .grid from disk
50 PRINT "At grid> type: basic run <file>"
60 END
```

---

## `basic compile <in> <out>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `basic compile <in> <out>` |
| **Purpose** | Compile file |
| **Action** | Produces .grid bytecode |
| **Sample** | `programs/encyclopedia/cmd-basic-compile.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: basic compile <in> <out>
20 REM Where: grid> shell
30 REM Purpose: Compile file
40 REM Action: Produces .grid bytecode
50 PRINT "At grid> type: basic compile <in> <out>"
60 END
```

---

## `basic mod run <name>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `basic mod run <name>` |
| **Purpose** | Run module |
| **Action** | Executes IDE package module |
| **Sample** | `programs/encyclopedia/cmd-basic-mod-run.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: basic mod run <name>
20 REM Where: grid> shell
30 REM Purpose: Run module
40 REM Action: Executes IDE package module
50 PRINT "At grid> type: basic mod run <name>"
60 END
```

---

## `basic samples`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `basic samples` |
| **Purpose** | List samples |
| **Action** | Flynn disk .bas listing |
| **Sample** | `programs/encyclopedia/cmd-basic-samples.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: basic samples
20 REM Where: grid> shell
30 REM Purpose: List samples
40 REM Action: Flynn disk .bas listing
50 PRINT "At grid> type: basic samples"
60 END
```

---

## `tutorial`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `tutorial` |
| **Purpose** | Run tutorial |
| **Action** | Executes /programs/tutorial.bas |
| **Sample** | `programs/encyclopedia/cmd-tutorial.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: tutorial
20 REM Where: grid> shell
30 REM Purpose: Run tutorial
40 REM Action: Executes /programs/tutorial.bas
50 PRINT "At grid> type: tutorial"
60 END
```

---

## `samples`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `samples` |
| **Purpose** | List samples |
| **Action** | Same as basic samples listing |
| **Sample** | `programs/encyclopedia/cmd-samples.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: samples
20 REM Where: grid> shell
30 REM Purpose: List samples
40 REM Action: Same as basic samples listing
50 PRINT "At grid> type: samples"
60 END
```

---

## `basictest`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `basictest` |
| **Purpose** | Interpreter test |
| **Action** | Deterministic self-test (OK15) |
| **Sample** | `programs/encyclopedia/cmd-basictest.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: basictest
20 REM Where: grid> shell
30 REM Purpose: Interpreter test
40 REM Action: Deterministic self-test (OK15)
50 PRINT "At grid> type: basictest"
60 END
```

---

## `redteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `redteam` |
| **Purpose** | Red team lab |
| **Action** | Lists /programs/redteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-redteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: redteam
20 REM Where: grid> shell
30 REM Purpose: Red team lab
40 REM Action: Lists /programs/redteam/ demos
50 PRINT "At grid> type: redteam"
60 END
```

---

## `blackhat`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `blackhat` |
| **Purpose** | Black hat lab |
| **Action** | Lists /programs/blackhat/ demos |
| **Sample** | `programs/encyclopedia/cmd-blackhat.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: blackhat
20 REM Where: grid> shell
30 REM Purpose: Black hat lab
40 REM Action: Lists /programs/blackhat/ demos
50 PRINT "At grid> type: blackhat"
60 END
```

---

## `whiteteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `whiteteam` |
| **Purpose** | White team lab |
| **Action** | Lists /programs/whiteteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-whiteteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: whiteteam
20 REM Where: grid> shell
30 REM Purpose: White team lab
40 REM Action: Lists /programs/whiteteam/ demos
50 PRINT "At grid> type: whiteteam"
60 END
```

---

## `blueteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `blueteam` |
| **Purpose** | Blue team lab |
| **Action** | Lists /programs/blueteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-blueteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: blueteam
20 REM Where: grid> shell
30 REM Purpose: Blue team lab
40 REM Action: Lists /programs/blueteam/ demos
50 PRINT "At grid> type: blueteam"
60 END
```

---

## `purpleteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `purpleteam` |
| **Purpose** | Purple team lab |
| **Action** | Lists /programs/purpleteam/ chains |
| **Sample** | `programs/encyclopedia/cmd-purpleteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: purpleteam
20 REM Where: grid> shell
30 REM Purpose: Purple team lab
40 REM Action: Lists /programs/purpleteam/ chains
50 PRINT "At grid> type: purpleteam"
60 END
```

---

## `greenteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `greenteam` |
| **Purpose** | Green hat lab |
| **Action** | Lists /programs/greenteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-greenteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: greenteam
20 REM Where: grid> shell
30 REM Purpose: Green hat lab
40 REM Action: Lists /programs/greenteam/ demos
50 PRINT "At grid> type: greenteam"
60 END
```

---

## `yellowteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `yellowteam` |
| **Purpose** | Yellow hat lab |
| **Action** | Lists /programs/yellowteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-yellowteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: yellowteam
20 REM Where: grid> shell
30 REM Purpose: Yellow hat lab
40 REM Action: Lists /programs/yellowteam/ demos
50 PRINT "At grid> type: yellowteam"
60 END
```

---

## `orangeteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `orangeteam` |
| **Purpose** | Orange hat lab |
| **Action** | Lists /programs/orangeteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-orangeteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: orangeteam
20 REM Where: grid> shell
30 REM Purpose: Orange hat lab
40 REM Action: Lists /programs/orangeteam/ demos
50 PRINT "At grid> type: orangeteam"
60 END
```

---

## `greyteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `greyteam` |
| **Purpose** | Grey hat lab |
| **Action** | Lists /programs/greyteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-greyteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: greyteam
20 REM Where: grid> shell
30 REM Purpose: Grey hat lab
40 REM Action: Lists /programs/greyteam/ demos
50 PRINT "At grid> type: greyteam"
60 END
```

---

## `daemonteam`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `daemonteam` |
| **Purpose** | Flynn daemon lab |
| **Action** | Lists /programs/daemonteam/ demos |
| **Sample** | `programs/encyclopedia/cmd-daemonteam.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: daemonteam
20 REM Where: grid> shell
30 REM Purpose: Flynn daemon lab
40 REM Action: Lists /programs/daemonteam/ demos
50 PRINT "At grid> type: daemonteam"
60 END
```

---

## `ai`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ai` |
| **Purpose** | AI command summary |
| **Action** | Shows ai subcommands |
| **Sample** | `programs/encyclopedia/cmd-ai.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ai
20 REM Where: grid> shell
30 REM Purpose: AI command summary
40 REM Action: Shows ai subcommands
50 PRINT "At grid> type: ai"
60 END
```

---

## `ai ask <prompt>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ai ask <prompt>` |
| **Purpose** | Ask AI |
| **Action** | Sends prompt to bridge |
| **Sample** | `programs/encyclopedia/cmd-ai-ask.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ai ask <prompt>
20 REM Where: grid> shell
30 REM Purpose: Ask AI
40 REM Action: Sends prompt to bridge
50 PRINT "At grid> type: ai ask <prompt>"
60 END
```

---

## `ai explain [line]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ai explain [line]` |
| **Purpose** | Explain BASIC line |
| **Action** | AI line explanation |
| **Sample** | `programs/encyclopedia/cmd-ai-explain.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ai explain [line]
20 REM Where: grid> shell
30 REM Purpose: Explain BASIC line
40 REM Action: AI line explanation
50 PRINT "At grid> type: ai explain [line]"
60 END
```

---

## `ai fix <code>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ai fix <code>` |
| **Purpose** | Fix BASIC code |
| **Action** | AI suggested fix |
| **Sample** | `programs/encyclopedia/cmd-ai-fix.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ai fix <code>
20 REM Where: grid> shell
30 REM Purpose: Fix BASIC code
40 REM Action: AI suggested fix
50 PRINT "At grid> type: ai fix <code>"
60 END
```

---

## `ai complete <code>`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ai complete <code>` |
| **Purpose** | Complete code |
| **Action** | AI completion |
| **Sample** | `programs/encyclopedia/cmd-ai-complete.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ai complete <code>
20 REM Where: grid> shell
30 REM Purpose: Complete code
40 REM Action: AI completion
50 PRINT "At grid> type: ai complete <code>"
60 END
```

---

## `ai models`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ai models` |
| **Purpose** | AI model info |
| **Action** | Bridge model listing |
| **Sample** | `programs/encyclopedia/cmd-ai-models.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ai models
20 REM Where: grid> shell
30 REM Purpose: AI model info
40 REM Action: Bridge model listing
50 PRINT "At grid> type: ai models"
60 END
```

---

## `btc`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `btc` |
| **Purpose** | Bitcoin summary |
| **Action** | Shows btc subcommands |
| **Sample** | `programs/encyclopedia/cmd-btc.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: btc
20 REM Where: grid> shell
30 REM Purpose: Bitcoin summary
40 REM Action: Shows btc subcommands
50 PRINT "At grid> type: btc"
60 END
```

---

## `btc status`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `btc status` |
| **Purpose** | BTC bridge status |
| **Action** | Connection to host node |
| **Sample** | `programs/encyclopedia/cmd-btc-status.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: btc status
20 REM Where: grid> shell
30 REM Purpose: BTC bridge status
40 REM Action: Connection to host node
50 PRINT "At grid> type: btc status"
60 END
```

---

## `btc balance`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `btc balance` |
| **Purpose** | Wallet balance |
| **Action** | getbalance via bridge |
| **Sample** | `programs/encyclopedia/cmd-btc-balance.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: btc balance
20 REM Where: grid> shell
30 REM Purpose: Wallet balance
40 REM Action: getbalance via bridge
50 PRINT "At grid> type: btc balance"
60 END
```

---

## `btc info`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `btc info` |
| **Purpose** | Blockchain info |
| **Action** | getblockchaininfo RPC |
| **Sample** | `programs/encyclopedia/cmd-btc-info.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: btc info
20 REM Where: grid> shell
30 REM Purpose: Blockchain info
40 REM Action: getblockchaininfo RPC
50 PRINT "At grid> type: btc info"
60 END
```

---

## `btc network`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `btc network` |
| **Purpose** | Network info |
| **Action** | getnetworkinfo RPC |
| **Sample** | `programs/encyclopedia/cmd-btc-network.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: btc network
20 REM Where: grid> shell
30 REM Purpose: Network info
40 REM Action: getnetworkinfo RPC
50 PRINT "At grid> type: btc network"
60 END
```

---

## `btc call <method> [json]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `btc call <method> [json]` |
| **Purpose** | Arbitrary RPC |
| **Action** | Generic JSON-RPC call |
| **Sample** | `programs/encyclopedia/cmd-btc-call.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: btc call <method> [json]
20 REM Where: grid> shell
30 REM Purpose: Arbitrary RPC
40 REM Action: Generic JSON-RPC call
50 PRINT "At grid> type: btc call <method> [json]"
60 END
```

---

## `theme flynn`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `theme flynn` |
| **Purpose** | Flynn theme |
| **Action** | Cyan shell prompt colors |
| **Sample** | `programs/encyclopedia/cmd-theme-flynn.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: theme flynn
20 REM Where: grid> shell
30 REM Purpose: Flynn theme
40 REM Action: Cyan shell prompt colors
50 PRINT "At grid> type: theme flynn"
60 END
```

---

## `theme clu`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `theme clu` |
| **Purpose** | CLU theme |
| **Action** | Red shell prompt colors |
| **Sample** | `programs/encyclopedia/cmd-theme-clu.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: theme clu
20 REM Where: grid> shell
30 REM Purpose: CLU theme
40 REM Action: Red shell prompt colors
50 PRINT "At grid> type: theme clu"
60 END
```

---

## `recognizer`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `recognizer` |
| **Purpose** | Patrol easter egg |
| **Action** | Recognizer flyover animation |
| **Sample** | `programs/encyclopedia/cmd-recognizer.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: recognizer
20 REM Where: grid> shell
30 REM Purpose: Patrol easter egg
40 REM Action: Recognizer flyover animation
50 PRINT "At grid> type: recognizer"
60 END
```

---

## `recognizer start`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `recognizer start` |
| **Purpose** | Start patrol |
| **Action** | Background recognizer service |
| **Sample** | `programs/encyclopedia/cmd-recognizer-start.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: recognizer start
20 REM Where: grid> shell
30 REM Purpose: Start patrol
40 REM Action: Background recognizer service
50 PRINT "At grid> type: recognizer start"
60 END
```

---

## `recognizer stop`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `recognizer stop` |
| **Purpose** | Stop patrol |
| **Action** | Stops recognizer service |
| **Sample** | `programs/encyclopedia/cmd-recognizer-stop.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: recognizer stop
20 REM Where: grid> shell
30 REM Purpose: Stop patrol
40 REM Action: Stops recognizer service
50 PRINT "At grid> type: recognizer stop"
60 END
```

---

## `ide [file]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `ide [file]` |
| **Purpose** | Grid Workbench |
| **Action** | Opens GEM + AmigaDOS workshop |
| **Sample** | `programs/encyclopedia/cmd-ide.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: ide [file]
20 REM Where: grid> shell
30 REM Purpose: Grid Workbench
40 REM Action: Opens GEM + AmigaDOS workshop
50 PRINT "At grid> type: ide [file]"
60 END
```

---

## `workshop [file]`

| Field | Value |
|-------|-------|
| **Where** | grid> shell (Esc from IDE) |
| **Syntax** | `workshop [file]` |
| **Purpose** | Grid Workbench |
| **Action** | Alias for ide command |
| **Sample** | `programs/encyclopedia/cmd-workshop.bas` |
| **See also** | [shell-from-ide.md](../shell-from-ide.md) |

```basic
10 REM Encyclopedia: workshop [file]
20 REM Where: grid> shell
30 REM Purpose: Grid Workbench
40 REM Action: Alias for ide command
50 PRINT "At grid> type: workshop [file]"
60 END
```

---
