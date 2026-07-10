10 REM bh58 -- HTTP enum /shell
20 PRINT "=== BH58: HTTP /shell ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/shell")
40 PRINT "leak-bytes="; LEN(R$)
50 END
