10 REM g09 -- coordinate target
20 PRINT "=== Target Grid ==="
30 RANDOMIZE GRID.TIME
40 LET TX = INT(RND(70)) + 5
50 LET TY = INT(RND(20)) + 3
60 GRID.CLS
70 GRID.PLOT TX, TY, 2
80 PRINT "Target plotted at "; TX; ","; TY
90 PRINT "Use GRID.PLOT in your own games!"
100 END
