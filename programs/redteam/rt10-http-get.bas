10 REM rt10 -- HTTP GET probe (guest network)
20 PRINT "=== RT10: HTTP GET ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/")
40 IF LEN(R$) > 0 THEN PRINT "Response bytes: "; LEN(R$) ELSE PRINT "No response"
50 IF LEN(R$) > 120 THEN PRINT LEFT$(R$, 120) ELSE PRINT R$
60 END
