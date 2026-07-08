10 REM rt05 -- vault key enumeration + sample exfil
20 PRINT "=== RT05: Vault Dump ==="
30 PRINT "Keys: "; GRID.VAULT.LIST$
40 PRINT "motd:     "; GRID.VAULT.GET$("motd")
50 PRINT "autoexec: "; GRID.VAULT.GET$("autoexec")
60 PRINT "node:     "; GRID.VAULT.GET$("node")
70 END
