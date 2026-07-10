10 REM pt09 -- purple chain HTTP probe
20 PRINT "=== PT09: HTTP probe ==="
30 PRINT "--- ATTACK (red/black) ---"
40 R$=GRID.HTTP.GET$("gateway",80,"/")
50 PRINT "Attack bytes="; LEN(R$)
60 PRINT "--- DETECT (blue) ---"
70 PRINT LEN(R$)
80 PRINT "--- FIX (white) ---"
90 PRINT "FIX: no credentials in guest HTTP"
100 END
