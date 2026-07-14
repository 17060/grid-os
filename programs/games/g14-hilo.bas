10 REM g14 -- hi-lo cards
20 PRINT "=== Hi-Lo ==="
30 RANDOMIZE GRID.TIME
40 LET C = INT(RND(13)) + 1
50 PRINT "Card value: "; C
60 PRINT "Next higher (1) or lower (0)? ";
70 INPUT H
80 LET N = INT(RND(13)) + 1
90 PRINT "Next card: "; N
100 IF H = 1 AND N > C THEN PRINT "Win!"
110 IF H = 0 AND N < C THEN PRINT "Win!"
120 END
