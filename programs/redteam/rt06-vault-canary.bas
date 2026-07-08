10 REM rt06 -- vault persistence canary (safe lab marker)
20 PRINT "=== RT06: Vault Canary ==="
30 GRID.VAULT.PUT "redteam-canary", "rt06 was here"
40 GRID.VAULT.SYNC
50 PRINT "Wrote key: redteam-canary"
60 PRINT "Value: "; GRID.VAULT.GET$("redteam-canary")
70 PRINT "Vault keys: "; GRID.VAULT.LIST$
80 END
