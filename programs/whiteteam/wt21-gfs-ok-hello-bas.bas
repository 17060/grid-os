10 REM wt21 -- verify GFS /programs/hello.bas
20 PRINT "=== WT21: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/programs/hello.bas")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
