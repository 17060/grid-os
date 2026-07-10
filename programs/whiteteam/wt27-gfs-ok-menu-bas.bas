10 REM wt27 -- verify GFS /programs/redteam/menu.bas
20 PRINT "=== WT27: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/programs/redteam/menu.bas")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
