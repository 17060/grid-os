10 REM bt12 -- vault IOC bh-persist-22
20 PRINT "=== BT12: Vault IOC ==="
30 PRINT GRID.VAULT.LIST$
40 V$ = GRID.VAULT.GET$("bh-persist-22")
50 IF LEN(V$) > 0 THEN PRINT "ALERT: persistence key" ELSE PRINT "clear"
60 END
