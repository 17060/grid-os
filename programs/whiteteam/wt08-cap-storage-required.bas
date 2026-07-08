10 REM wt08 -- verify STORAGE-required granted
20 PRINT "=== WT08: Cap compliance ==="
30 IF GRID.CAP(64) THEN PRINT "OK: STORAGE-required" ELSE PRINT "FAIL: STORAGE-required"
40 PRINT "Mask: "; GRID.CAPS$
50 END
