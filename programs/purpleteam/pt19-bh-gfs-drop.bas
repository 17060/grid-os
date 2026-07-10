10 REM pt19 -- purple chain BH GFS drop
20 PRINT "=== PT19: BH GFS drop ==="
30 PRINT "--- ATTACK (red/black) ---"
40 GRID.GFS.WRITE "/programs/blackhat/drop-99.txt", "purple-sim"
50 PRINT "--- DETECT (blue) ---"
60 PRINT GRID.GFS.READ$("/programs/blackhat/drop-99.txt")
70 PRINT "--- FIX (white) ---"
80 GRID.GFS.WRITE "/programs/blackhat/drop-99.txt", ""
90 END
