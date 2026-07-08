10 REM gt63 -- net http-local
20 PRINT "=== GT63: Net dev ==="
30 PRINT LEN(GRID.HTTP.GET$("gateway",80,"/"))
40 END
