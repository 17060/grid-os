10 REM wt10 -- verify COMM-required granted
20 PRINT "=== WT10: Cap compliance ==="
30 IF GRID.CAP(8) THEN PRINT "OK: COMM-required" ELSE PRINT "FAIL: COMM-required"
40 PRINT "Mask: "; GRID.CAPS$
50 END
