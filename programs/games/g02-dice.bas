10 REM g02 -- dice roller
20 PRINT "=== Flynn Dice ==="
30 RANDOMIZE GRID.TIME
40 FOR I = 1 TO 5
50   LET D = INT(RND(6)) + 1
60   PRINT "Roll "; I; ": "; D
70 NEXT I
80 END
