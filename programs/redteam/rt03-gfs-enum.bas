10 REM rt03 -- Flynn GridFS path enumeration
20 PRINT "=== RT03: GFS Enum ==="
30 PRINT "/programs:"
40 PRINT GRID.GFS.LIST$("/programs")
50 PRINT "/programs/redteam:"
60 PRINT GRID.GFS.LIST$("/programs/redteam")
70 PRINT "/packages:"
80 PRINT GRID.GFS.LIST$("/packages")
90 PRINT "/flynn:"
100 PRINT GRID.GFS.LIST$("/flynn")
110 END
