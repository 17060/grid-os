10 REM pt22 -- purple chain host bridges
20 PRINT "=== PT22: host bridges ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT GRID.BTC.STATUS$
50 PRINT GRID.AI.MODELS$
60 PRINT GRID.PING("10.0.2.2")
70 PRINT "--- DETECT (blue) ---"
80 PRINT GRID.BTC.STATUS$
90 PRINT "--- FIX (white) ---"
100 PRINT "FIX: bind bridges to 127.0.0.1"
110 END
