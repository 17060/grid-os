10 REM rt67 -- HTTP GET /
20 PRINT "=== RT67: HTTP / ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/")
40 PRINT "bytes="; LEN(R$)
50 END
