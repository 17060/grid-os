10 REM gy71 -- gray persistence
20 PRINT "=== GY71: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-71", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-71")
60 END
