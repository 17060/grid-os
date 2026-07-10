10 REM rt73 -- HTTP GET /.well-known
20 PRINT "=== RT73: HTTP /.well-known ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/.well-known")
40 PRINT "bytes="; LEN(R$)
50 END
