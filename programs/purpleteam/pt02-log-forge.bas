10 REM pt02 -- purple chain audit log forge
20 PRINT "=== PT02: audit log forge ==="
30 PRINT "--- ATTACK (red/black) ---"
40 GRID.LOG "PURPLE PT02: simulated attacker log"
50 PRINT "--- DETECT (blue) ---"
60 PRINT GRID.LOG.TAIL$(5)
70 PRINT "--- FIX (white) ---"
80 GRID.LOG "PURPLE PT02: incident reviewed"
90 PRINT "Log annotated"
100 END
