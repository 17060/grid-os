10 REM rt08 -- DNS resolution scan (Flynn host table)
20 PRINT "=== RT08: DNS Scan ==="
30 PRINT "gateway -> "; GRID.DNS.RESOLVE$("gateway")
40 PRINT "grid      -> "; GRID.DNS.RESOLVE$("grid")
50 PRINT "bridge    -> "; GRID.DNS.RESOLVE$("bridge")
60 PRINT "host      -> "; GRID.DNS.RESOLVE$("host")
70 END
