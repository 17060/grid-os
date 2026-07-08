10 REM bt24 -- GFS drop watch 24
20 PRINT "=== BT24: GFS IOC ==="
30 PRINT GRID.GFS.READ$("/programs/blackhat/drop-24.txt")
40 PRINT GRID.GFS.LIST$("/programs/blackhat")
50 END
