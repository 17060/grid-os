10 REM bh22 -- persistence drop 22
20 PRINT "=== BH22: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-22", "owned-22"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-22.txt", "bh22"
60 PRINT GRID.VAULT.GET$("bh-persist-22")
70 END
