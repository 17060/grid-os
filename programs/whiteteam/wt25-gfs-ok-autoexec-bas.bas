10 REM wt25 -- verify GFS /programs/autoexec.bas
20 PRINT "=== WT25: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/programs/autoexec.bas")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
