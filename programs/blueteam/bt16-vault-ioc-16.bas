10 REM bt16 -- vault IOC bh-persist-26
20 PRINT "=== BT16: Vault IOC ==="
30 PRINT GRID.VAULT.LIST$
40 V$ = GRID.VAULT.GET$("bh-persist-26")
50 IF LEN(V$) > 0 THEN PRINT "ALERT: persistence key" ELSE PRINT "clear"
60 END
