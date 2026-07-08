10 REM gy35 -- HTTP gray enum /login
20 PRINT "=== GY35: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/login"))
40 END
