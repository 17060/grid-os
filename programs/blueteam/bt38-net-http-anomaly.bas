10 REM bt38 -- net monitor http-anomaly
20 PRINT "=== BT38: Net monitor ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/"))
40 END
