10 REM pt23 -- purple chain full purple loop
20 PRINT "=== PT23: full purple loop ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT GRID.WHOAMI$
50 PRINT GRID.VAULT.LIST$
60 PRINT "--- DETECT (blue) ---"
70 PRINT GRID.LOG.TAIL$(8)
80 PRINT GRID.JOBS.LIST$
90 PRINT "--- FIX (white) ---"
100 GRID.VAULT.SYNC
110 PRINT "Compliance sync done"
120 END
