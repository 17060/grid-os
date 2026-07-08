10 REM bh53 -- HTTP enum /api
20 PRINT "=== BH53: HTTP /api ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/api")
40 PRINT "leak-bytes="; LEN(R$)
50 END
