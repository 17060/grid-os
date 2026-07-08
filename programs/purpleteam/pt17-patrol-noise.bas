10 REM pt17 -- purple chain recognizer patrol
20 PRINT "=== PT17: recognizer patrol ==="
30 PRINT "--- ATTACK (red/black) ---"
40 GRID.RECOGNIZER.START
50 PRINT GRID.RECOGNIZER.STATUS$
60 PRINT "--- DETECT (blue) ---"
70 PRINT GRID.RECOGNIZER.STATUS$
80 PRINT "--- FIX (white) ---"
90 GRID.RECOGNIZER.STOP
100 PRINT "FIX: patrol stopped"
110 END
