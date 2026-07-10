10 REM rt98 -- combo gfs-write-read
20 PRINT "=== RT98: gfs-write-read ==="
30 GRID.GFS.WRITE "/programs/redteam/rt98.txt","ok"
40 PRINT GRID.GFS.READ$("/programs/redteam/rt98.txt")
50 END
