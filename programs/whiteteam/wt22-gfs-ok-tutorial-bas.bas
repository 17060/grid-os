10 REM wt22 -- verify GFS /programs/tutorial.bas
20 PRINT "=== WT22: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/programs/tutorial.bas")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
