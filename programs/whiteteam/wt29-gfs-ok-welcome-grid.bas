10 REM wt29 -- verify GFS /source/welcome.grid
20 PRINT "=== WT29: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/source/welcome.grid")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
