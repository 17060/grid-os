10 REM bh41 -- host scan gateway
20 PRINT "=== BH41: SCAN gateway ==="
30 PRINT GRID.DNS.RESOLVE$("gateway")
40 PRINT GRID.PING("gateway")
50 END
