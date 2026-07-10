10 REM bh23 -- persistence drop 23
20 PRINT "=== BH23: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-23", "owned-23"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-23.txt", "bh23"
60 PRINT GRID.VAULT.GET$("bh-persist-23")
70 END
