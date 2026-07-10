10 REM gy80 -- gray persistence
20 PRINT "=== GY80: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-80", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-80")
60 END
