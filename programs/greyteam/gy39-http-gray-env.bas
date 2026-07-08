10 REM gy39 -- HTTP gray enum /.env
20 PRINT "=== GY39: HTTP gray ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/.env"))
40 END
