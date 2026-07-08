10 REM rt69 -- HTTP GET /robots.txt
20 PRINT "=== RT69: HTTP /robots.txt ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/robots.txt")
40 PRINT "bytes="; LEN(R$)
50 END
