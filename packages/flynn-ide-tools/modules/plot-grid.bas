10 REM IDE module: plot-grid
20 GRID.CLS
30 FOR X = 0 TO 40
40   GRID.PLOT X, X, 2
50   GRID.PLOT 80 - X, X, 3
60 NEXT X
70 PRINT "Plot demo complete."
80 END
