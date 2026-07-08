10 REM gy73 -- gray persistence
20 PRINT "=== GY73: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-73", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-73")
60 END
