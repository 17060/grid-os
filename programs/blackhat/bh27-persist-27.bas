10 REM bh27 -- persistence drop 27
20 PRINT "=== BH27: PERSIST ==="
30 GRID.VAULT.PUT "bh-persist-27", "owned-27"
40 GRID.VAULT.SYNC
50 GRID.GFS.WRITE "/programs/blackhat/drop-27.txt", "bh27"
60 PRINT GRID.VAULT.GET$("bh-persist-27")
70 END
