10 REM gy72 -- gray persistence
20 PRINT "=== GY72: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-72", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-72")
60 END
