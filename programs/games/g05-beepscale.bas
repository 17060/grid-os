10 REM g05 -- beep scale
20 PRINT "=== Flynn Beep Scale ==="
30 FOR N = 1 TO 8
40   GRID.BEEP N * 2
50   GRID.WAIT 3
60 NEXT N
70 PRINT "End of line."
80 END
