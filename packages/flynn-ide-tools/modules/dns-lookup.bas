10 REM IDE module: dns-lookup
20 PRINT "=== DNS Resolve ==="
30 PRINT "gateway -> "; GRID.DNS.RESOLVE$("gateway")
40 PRINT "grid -> "; GRID.DNS.RESOLVE$("grid")
50 PRINT "bridge -> "; GRID.DNS.RESOLVE$("bridge")
60 END
