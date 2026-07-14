10 REM g13 -- remember the number
20 PRINT "=== Memory Flash ==="
30 RANDOMIZE GRID.TIME
40 LET S = INT(RND(900)) + 100
50 PRINT "Remember: "; S
60 GRID.WAIT 8
70 GRID.CLS
80 PRINT "What was it? ";
90 INPUT G
100 IF G = S THEN PRINT "Sharp program!" ELSE PRINT "It was "; S
110 END
