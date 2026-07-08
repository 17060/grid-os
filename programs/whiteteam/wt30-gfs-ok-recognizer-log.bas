10 REM wt30 -- verify GFS /grid/recognizer.log
20 PRINT "=== WT30: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/grid/recognizer.log")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
