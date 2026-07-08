10 REM rt72 -- HTTP GET /health
20 PRINT "=== RT72: HTTP /health ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/health")
40 PRINT "bytes="; LEN(R$)
50 END
