10 REM bh21 -- persistence drop 21
20 PRINT "=== BH21: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-21", "owned-21"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-21.txt", "bh21"
60 PRINT GRID.VAULT.GET$("bh-persist-21")
70 END
