10 REM bh57 -- HTTP enum /.env
20 PRINT "=== BH57: HTTP /.env ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/.env")
40 PRINT "leak-bytes="; LEN(R$)
50 END
