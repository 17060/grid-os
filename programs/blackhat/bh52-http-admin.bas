10 REM bh52 -- HTTP enum /admin
20 PRINT "=== BH52: HTTP /admin ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/admin")
40 PRINT "leak-bytes="; LEN(R$)
50 END
