10 REM bh59 -- HTTP enum /debug
20 PRINT "=== BH59: HTTP /debug ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/debug")
40 PRINT "leak-bytes="; LEN(R$)
50 END
