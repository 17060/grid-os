10 REM rt74 -- HTTP GET /admin
20 PRINT "=== RT74: HTTP /admin ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/admin")
40 PRINT "bytes="; LEN(R$)
50 END
