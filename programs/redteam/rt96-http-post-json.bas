10 REM rt96 -- combo http-post-json
20 PRINT "=== RT96: http-post-json ==="
30 R$=GRID.HTTP.POST$("gateway",80,"/","{\"probe\":1}")
40 PRINT LEN(R$)
50 END
