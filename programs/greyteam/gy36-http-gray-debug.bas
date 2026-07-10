10 REM gy36 -- HTTP gray enum /debug
20 PRINT "=== GY36: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/debug"))
40 END
