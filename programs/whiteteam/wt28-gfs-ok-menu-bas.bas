10 REM wt28 -- verify GFS /programs/blackhat/menu.bas
20 PRINT "=== WT28: GFS integrity ==="
30 R$ = GRID.GFS.READ$("/programs/blackhat/menu.bas")
40 IF LEN(R$) > 0 THEN PRINT "OK: file present" ELSE PRINT "MISSING"
50 END
