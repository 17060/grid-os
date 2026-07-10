10 REM bh30 -- persistence drop 30
20 PRINT "=== BH30: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-30", "owned-30"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-30.txt", "bh30"
60 PRINT GRID.VAULT.GET$("bh-persist-30")
70 END
