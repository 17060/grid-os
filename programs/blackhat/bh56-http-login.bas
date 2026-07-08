10 REM bh56 -- HTTP enum /login
20 PRINT "=== BH56: HTTP /login ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/login")
40 PRINT "leak-bytes="; LEN(R$)
50 END
