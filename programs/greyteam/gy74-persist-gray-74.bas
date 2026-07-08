10 REM gy74 -- gray persistence
20 PRINT "=== GY74: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-74", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-74")
60 END
