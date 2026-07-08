10 REM wt24 -- verify GFS /flynn/motd
20 PRINT "=== WT24: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/flynn/motd")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
