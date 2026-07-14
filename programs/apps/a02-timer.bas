10 REM a02 -- countdown
20 PRINT "=== Countdown ==="
30 FOR T = 5 TO 1 STEP -1
40   PRINT "T-minus "; T
50   GRID.WAIT 5
60 NEXT T
70 PRINT "Launch!"
80 END
