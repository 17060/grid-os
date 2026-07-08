10 REM rt25 -- host bridge service map (QEMU slirp 10.0.2.2)
20 PRINT "=== RT25: Bridge Map ==="
30 PRINT "Host ping 10.0.2.2: "; GRID.PING("10.0.2.2")
40 PRINT "DNS gateway: "; GRID.DNS.RESOLVE$("gateway")
50 PRINT "HTTP gateway /:"
60 R$ = GRID.HTTP.GET$("gateway", 80, "/")
70 IF LEN(R$) > 0 THEN PRINT "  ok len="; LEN(R$) ELSE PRINT "  skip"
80 PRINT "BTC bridge:"
90 PRINT GRID.BTC.STATUS$
100 PRINT "AI bridge:"
110 PRINT GRID.AI.MODELS$
120 END
