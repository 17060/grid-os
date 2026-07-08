10 REM gy79 -- gray persistence
20 PRINT "=== GY79: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-79", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-79")
60 END
