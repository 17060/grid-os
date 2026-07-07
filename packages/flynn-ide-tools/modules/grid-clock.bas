10 REM IDE module: grid-clock
20 PRINT "=== Grid Clock ==="
30 PRINT "Ticks: "; GRID.TIME
40 FOR I = 1 TO 3
50   PRINT "  beat "; I; " @ "; GRID.TIME
60   GRID.WAIT 5
70 NEXT I
80 END
