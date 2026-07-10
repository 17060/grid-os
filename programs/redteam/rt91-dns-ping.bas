10 REM rt91 -- combo dns-ping
20 PRINT "=== RT91: dns-ping ==="
30 PRINT GRID.DNS.RESOLVE$("gateway")
40 PRINT GRID.PING("gateway")
50 END
