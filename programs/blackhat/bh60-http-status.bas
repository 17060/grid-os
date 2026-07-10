10 REM bh60 -- HTTP enum /status
20 PRINT "=== BH60: HTTP /status ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/status")
40 PRINT "leak-bytes="; LEN(R$)
50 END
