10 REM bh55 -- HTTP enum /backup
20 PRINT "=== BH55: HTTP /backup ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/backup")
40 PRINT "leak-bytes="; LEN(R$)
50 END
