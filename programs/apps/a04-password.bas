10 REM a04 -- password gen
20 PRINT "=== Password Gen ==="
30 RANDOMIZE GRID.TIME
40 PRINT "Pin: "; INT(RND(9000)) + 1000
50 END
