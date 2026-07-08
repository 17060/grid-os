10 REM rt09 -- ICMP ping sweep
20 PRINT "=== RT09: Ping Sweep ==="
30 PRINT "gateway: "; GRID.PING("gateway")
40 PRINT "grid:    "; GRID.PING("grid")
50 PRINT "bridge:  "; GRID.PING("bridge")
60 PRINT "10.0.2.2:"; GRID.PING("10.0.2.2")
70 END
