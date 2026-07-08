10 REM rt45 -- vault key redteam-canary
20 PRINT "=== RT45: vault/redteam-canary ==="
30 PRINT "keys: "; GRID.VAULT.LIST$
40 PRINT GRID.VAULT.GET$("redteam-canary")
50 END
