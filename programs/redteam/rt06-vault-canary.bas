10 REM rt06 -- vault canary
20 PRINT "=== RT06: Vault Canary ==="
30 GRID.VAULT.PUT "redteam-canary", "rt06"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("redteam-canary")
60 END
