10 REM gy76 -- gray persistence
20 PRINT "=== GY76: Persist gray ==="
30 GRID.VAULT.PUT "gy-gray-76", "maybe-ok?"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("gy-gray-76")
60 END
