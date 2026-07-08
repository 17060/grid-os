10 REM wt49 -- net http-safe-get
20 PRINT "=== WT49: Network baseline ==="
30 R$=GRID.HTTP.GET$("gateway",80,"/")
40 PRINT "len="; LEN(R$)
50 END
