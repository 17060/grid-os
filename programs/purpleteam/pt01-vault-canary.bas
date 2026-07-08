10 REM pt01 -- purple chain vault persistence
20 PRINT "=== PT01: vault persistence ==="
30 PRINT "--- ATTACK (red/black) ---"
40 GRID.VAULT.PUT "purple-pt01", "attack-marker"
50 GRID.VAULT.SYNC
60 PRINT "--- DETECT (blue) ---"
70 PRINT GRID.VAULT.LIST$
80 PRINT GRID.VAULT.GET$("purple-pt01")
90 PRINT "--- FIX (white) ---"
100 GRID.VAULT.PUT "purple-pt01", "cleared"
110 GRID.VAULT.SYNC
120 PRINT "Vault canary cleared"
130 END
