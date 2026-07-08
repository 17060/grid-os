10 REM wt07 -- verify READ-required granted
20 PRINT "=== WT07: Cap compliance ==="
30 IF GRID.CAP(1) THEN PRINT "OK: READ-required" ELSE PRINT "FAIL: READ-required"
40 PRINT "Mask: "; GRID.CAPS$
50 END
