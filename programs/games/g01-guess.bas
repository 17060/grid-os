10 REM g01 -- number guess
20 PRINT "=== Guess the Number (1-100) ==="
30 RANDOMIZE GRID.TIME
40 LET S = INT(RND(100)) + 1
50 LET T = 0
60 FOR R = 1 TO 8
70   PRINT "Guess? ";
80   INPUT G
90   LET T = T + 1
100   IF G = S THEN PRINT "You win in "; T; " tries!": END
110   IF G < S THEN PRINT "Higher..." ELSE PRINT "Lower..."
120 NEXT R
130 PRINT "The number was "; S
140 END
