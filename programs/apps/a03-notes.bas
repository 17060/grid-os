10 REM a03 -- quick note
20 PRINT "=== Flynn Notes ==="
30 GRID.VAULT.PUT "note", "Remember to save your programs!"
40 GRID.VAULT.SYNC
50 PRINT GRID.VAULT.GET$("note")
60 END
