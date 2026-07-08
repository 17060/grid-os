10 REM gt28 -- package manifest review
20 PRINT "=== GT28: Pkg integrity ==="
30 PRINT GRID.PKG.LIST$
40 M$ = GRID.GFS.READ$("/packages/flynn-ide-tools/MANIFEST")
50 PRINT "MANIFEST bytes="; LEN(M$)
60 END
