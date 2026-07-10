10 REM bh26 -- persistence drop 26
20 PRINT "=== BH26: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-26", "owned-26"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-26.txt", "bh26"
60 PRINT GRID.VAULT.GET$("bh-persist-26")
70 END
