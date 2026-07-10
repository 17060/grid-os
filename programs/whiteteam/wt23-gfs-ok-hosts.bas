10 REM wt23 -- verify GFS /etc/hosts
20 PRINT "=== WT23: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/etc/hosts")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
