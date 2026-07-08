10 REM pt03 -- purple chain GFS drop file
20 PRINT "=== PT03: GFS drop file ==="
30 PRINT "--- ATTACK (red/black) ---"
40 GRID.GFS.WRITE "/programs/purpleteam/pt03-drop.txt", "purple-drop"
50 PRINT "--- DETECT (blue) ---"
60 PRINT GRID.GFS.READ$("/programs/purpleteam/pt03-drop.txt")
70 PRINT "--- FIX (white) ---"
80 GRID.GFS.WRITE "/programs/purpleteam/pt03-drop.txt", ""
90 PRINT "Drop neutralized"
100 END
