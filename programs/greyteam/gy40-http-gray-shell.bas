10 REM gy40 -- HTTP gray enum /shell
20 PRINT "=== GY40: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/shell"))
40 END
