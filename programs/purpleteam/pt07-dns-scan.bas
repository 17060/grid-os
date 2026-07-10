10 REM pt07 -- purple chain DNS recon
20 PRINT "=== PT07: DNS recon ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT GRID.DNS.RESOLVE$("gateway")
50 PRINT GRID.DNS.RESOLVE$("bridge")
60 PRINT "--- DETECT (blue) ---"
70 PRINT GRID.PING("gateway")
80 PRINT "--- FIX (white) ---"
90 PRINT "FIX: DNS baseline documented"
100 END
