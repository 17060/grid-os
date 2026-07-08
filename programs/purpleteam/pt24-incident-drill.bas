10 REM pt24 -- purple chain incident response drill
20 PRINT "=== PT24: incident response drill ==="
30 PRINT "--- ATTACK (red/black) ---"
40 GRID.LOG "PURPLE PT24: simulated breach"
50 PRINT "--- DETECT (blue) ---"
60 PRINT GRID.LOG.TAIL$(4)
70 PRINT GRID.VAULT.LIST$
80 PRINT "--- FIX (white) ---"
90 GRID.LOG "PURPLE PT24: contained"
100 PRINT "IR drill complete"
110 END
