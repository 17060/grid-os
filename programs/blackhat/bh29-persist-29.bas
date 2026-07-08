10 REM bh29 -- persistence drop 29
20 PRINT "=== BH29: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-29", "owned-29"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-29.txt", "bh29"
60 PRINT GRID.VAULT.GET$("bh-persist-29")
70 END
