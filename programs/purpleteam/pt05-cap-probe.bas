10 REM pt05 -- purple chain capability probe
20 PRINT "=== PT05: capability probe ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT "Caps: "; GRID.CAPS$
50 PRINT "--- DETECT (blue) ---"
60 IF GRID.CAP(64) THEN PRINT "DETECT: STORAGE granted" ELSE PRINT "DETECT: no STORAGE"
70 PRINT "--- FIX (white) ---"
80 PRINT "FIX: least privilege for programs"
90 END
