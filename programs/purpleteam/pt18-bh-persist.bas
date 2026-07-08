10 REM pt18 -- purple chain BH vault persistence
20 PRINT "=== PT18: BH vault persistence ==="
30 PRINT "--- ATTACK (red/black) ---"
40 GRID.VAULT.PUT "bh-persist-99", "purple-sim"
50 GRID.VAULT.SYNC
60 PRINT "--- DETECT (blue) ---"
70 V$=GRID.VAULT.GET$("bh-persist-99")
80 IF LEN(V$)>0 THEN PRINT "ALERT: BH key"
90 PRINT "--- FIX (white) ---"
100 GRID.VAULT.PUT "bh-persist-99", ""
110 GRID.VAULT.SYNC
120 END
