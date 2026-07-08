10 REM rt23 -- GFS write drop (persistence canary file)
20 PRINT "=== RT23: GFS Drop ==="
30 GRID.GFS.WRITE "/programs/redteam/canary.txt", "redteam rt23 marker"
40 PRINT "Wrote /programs/redteam/canary.txt"
50 PRINT GRID.GFS.READ$("/programs/redteam/canary.txt")
60 END
