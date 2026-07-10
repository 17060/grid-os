10 REM bt25 -- GFS drop watch 25
20 PRINT "=== BT25: GFS IOC ==="
30 PRINT GRID.GFS.READ$("/programs/blackhat/drop-25.txt")
40 PRINT GRID.GFS.LIST$("/programs/blackhat")
50 END
