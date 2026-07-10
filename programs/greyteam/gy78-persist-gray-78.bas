10 REM gy78 -- gray persistence
20 PRINT "=== GY78: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-78", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-78")
60 END
