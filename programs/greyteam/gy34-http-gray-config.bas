10 REM gy34 -- HTTP gray enum /config
20 PRINT "=== GY34: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/config"))
40 END
