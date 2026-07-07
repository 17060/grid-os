10 REM IDE module: hosts-table
20 PRINT "=== /etc/hosts ==="
30 H$ = GRID.GFS.READ$("/etc/hosts")
40 IF LEN(H$) > 0 THEN PRINT H$ ELSE PRINT "(missing — gfs seed)"
50 END
