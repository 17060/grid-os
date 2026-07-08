10 REM gy38 -- HTTP gray enum /backup
20 PRINT "=== GY38: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/backup"))
40 END
