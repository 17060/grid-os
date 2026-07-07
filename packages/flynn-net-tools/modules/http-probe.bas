10 REM Flynn net-tools: http-probe
20 PRINT "=== HTTP Probe ==="
30 R$ = GRID.HTTP.GET$("gateway", 80, "/")
40 IF LEN(R$) > 0 THEN PRINT "HTTP ok ("; LEN(R$); " B)" ELSE PRINT "HTTP skip (no bridge)"
50 END
