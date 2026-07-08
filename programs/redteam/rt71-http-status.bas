10 REM rt71 -- HTTP GET /status
20 PRINT "=== RT71: HTTP /status ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/status")
40 PRINT "bytes="; LEN(R$)
50 END
