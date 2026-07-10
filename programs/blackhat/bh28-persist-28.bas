10 REM bh28 -- persistence drop 28
20 PRINT "=== BH28: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-28", "owned-28"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-28.txt", "bh28"
60 PRINT GRID.VAULT.GET$("bh-persist-28")
70 END
