10 REM bt30 -- GFS drop watch 30
20 PRINT "=== BT30: GFS IOC ==="
30 PRINT GRID.GFS.READ$("/programs/blackhat/drop-30.txt")
40 PRINT GRID.GFS.LIST$("/programs/blackhat")
50 END
