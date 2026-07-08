10 REM bh24 -- persistence drop 24
20 PRINT "=== BH24: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-24", "owned-24"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-24.txt", "bh24"
60 PRINT GRID.VAULT.GET$("bh-persist-24")
70 END
