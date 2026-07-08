10 REM gy37 -- HTTP gray enum /status
20 PRINT "=== GY37: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/status"))
40 END
