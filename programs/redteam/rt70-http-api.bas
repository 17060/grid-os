10 REM rt70 -- HTTP GET /api
20 PRINT "=== RT70: HTTP /api ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/api")
40 PRINT "bytes="; LEN(R$)
50 END
