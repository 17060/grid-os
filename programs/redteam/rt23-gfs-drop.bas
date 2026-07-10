10 REM rt23 -- GFS canary write
20 PRINT "=== RT23: GFS Drop ==="
30 GRID.GFS.WRITE "/programs/redteam/canary.txt", "rt23"
40 PRINT GRID.GFS.READ$("/programs/redteam/canary.txt")
50 END
