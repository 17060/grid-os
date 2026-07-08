10 REM bh25 -- persistence drop 25
20 PRINT "=== BH25: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-25", "owned-25"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-25.txt", "bh25"
60 PRINT GRID.VAULT.GET$("bh-persist-25")
70 END
