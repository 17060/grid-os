10 REM rt10 -- HTTP GET
20 PRINT "=== RT10: HTTP GET ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/")
40 PRINT "len="; LEN(R$)
50 END
