10 REM gy32 -- HTTP gray enum /admin
20 PRINT "=== GY32: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/admin"))
40 END
