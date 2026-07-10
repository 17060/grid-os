10 REM rt68 -- HTTP GET /index.html
20 PRINT "=== RT68: HTTP /index.html ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/index.html")
40 PRINT "bytes="; LEN(R$)
50 END
