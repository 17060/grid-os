10 REM dm24 -- http-daemon
20 PRINT "=== DM24: http-daemon ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/")
40 PRINT "len="; LEN(R$)
50 END
