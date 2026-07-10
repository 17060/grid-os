10 REM wt09 -- verify SPAWN-required granted
20 PRINT "=== WT09: Cap compliance ==="
30 IF GRID.CAP(4) THEN PRINT "OK: SPAWN-required" ELSE PRINT "FAIL: SPAWN-required"
40 PRINT "Mask: "; GRID.CAPS$
50 END
