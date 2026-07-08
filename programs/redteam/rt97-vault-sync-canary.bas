10 REM rt97 -- combo vault-sync-canary
20 PRINT "=== RT97: vault-sync-canary ==="
30 GRID.VAULT.PUT "rt97", "canary"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("rt97")
60 END
