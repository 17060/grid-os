10 REM pt20 -- purple chain network recon
20 PRINT "=== PT20: network recon ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT GRID.NET.STATUS$
50 PRINT GRID.PING("bridge")
60 PRINT "--- DETECT (blue) ---"
70 PRINT GRID.NET.STATUS$
80 PRINT "--- FIX (white) ---"
90 PRINT "FIX: net baseline saved"
100 END
