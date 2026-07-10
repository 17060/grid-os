10 REM wt26 -- verify GFS /packages/flynn-ide-tools/MANIFEST
20 PRINT "=== WT26: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/packages/flynn-ide-tools/MANIFEST")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
