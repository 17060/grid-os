10 REM bh51 -- HTTP enum /
20 PRINT "=== BH51: HTTP / ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/")
40 PRINT "leak-bytes="; LEN(R$)
50 END
