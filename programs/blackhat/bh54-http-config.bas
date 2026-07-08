10 REM bh54 -- HTTP enum /config
20 PRINT "=== BH54: HTTP /config ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/config")
40 PRINT "leak-bytes="; LEN(R$)
50 END
