10 REM bt13 -- vault IOC bh-persist-23
20 PRINT "=== BT13: Vault IOC ==="
30 PRINT GRID.VAULT.LIST$
40 V$ = GRID.VAULT.GET$("bh-persist-23")
50 IF LEN(V$) > 0 THEN PRINT "ALERT: persistence key" ELSE PRINT "clear"
60 END
