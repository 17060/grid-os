10 REM rt42 -- vault key motd
20 PRINT "=== RT42: vault/motd ==="
30 PRINT "keys: "; GRID.VAULT.LIST$
40 PRINT GRID.VAULT.GET$("motd")
50 END
