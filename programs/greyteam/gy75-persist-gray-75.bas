10 REM gy75 -- gray persistence
20 PRINT "=== GY75: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-75", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-75")
60 END
