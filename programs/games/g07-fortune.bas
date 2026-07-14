10 REM g07 -- fortune cookie
20 PRINT "=== Flynn Fortune ==="
30 RANDOMIZE GRID.TIME
40 LET F = INT(RND(5)) + 1
50 IF F = 1 THEN PRINT "The Grid favors the bold."
60 IF F = 2 THEN PRINT "End of line — but a new cycle begins."
70 IF F = 3 THEN PRINT "Save your work. Flynn always does."
80 IF F = 4 THEN PRINT "Type :run and see what happens."
90 IF F = 5 THEN PRINT "Programs are spells. Cast wisely."
100 END
