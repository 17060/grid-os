10 REM rt11 -- HTTP POST probe
20 PRINT "=== RT11: HTTP POST ==="
30 B$ = "redteam=probe&grid=1"
40 R$ = GRID.HTTP.POST$("gateway", 80, "/", B$)
50 IF LEN(R$) > 0 THEN PRINT "POST ok, len="; LEN(R$) ELSE PRINT "POST failed/skipped"
60 END
