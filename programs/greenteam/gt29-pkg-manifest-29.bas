10 REM gt29 -- package manifest review
20 PRINT "=== GT29: Pkg integrity ==="
30 PRINT GRID.PKG.LIST$
40 M$ = GRID.GFS.READ$("/packages/flynn-ide-tools/MANIFEST")
50 PRINT "MANIFEST bytes="; LEN(M$)
60 END
