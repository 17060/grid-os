10 REM rt11 -- HTTP POST
20 PRINT "=== RT11: HTTP POST ==="
30 R$ = GRID.HTTP.POST$("gateway", 80, "/", "probe=1")
40 PRINT "len="; LEN(R$)
50 END
