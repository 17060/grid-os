10 REM g08 -- math quiz
20 PRINT "=== Flynn Math Quiz ==="
30 RANDOMIZE GRID.TIME
40 LET A = INT(RND(12)) + 1
50 LET B = INT(RND(12)) + 1
60 PRINT "What is "; A; " + "; B; "?";
70 INPUT ANS
80 IF ANS = A + B THEN PRINT "Correct!" ELSE PRINT "Answer: "; A + B
90 END
