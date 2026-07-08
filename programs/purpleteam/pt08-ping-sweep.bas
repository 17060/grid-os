10 REM pt08 -- purple chain ping sweep
20 PRINT "=== PT08: ping sweep ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT GRID.PING("gateway")
50 PRINT GRID.PING("10.0.2.2")
60 PRINT "--- DETECT (blue) ---"
70 PRINT GRID.NET.STATUS$
80 PRINT "--- FIX (white) ---"
90 PRINT "FIX: alert on new live hosts"
100 END
