10 REM gy77 -- gray persistence
20 PRINT "=== GY77: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-77", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-77")
60 END
