10 REM bt19 -- vault IOC bh-persist-29
20 PRINT "=== BT19: Vault IOC ==="
30 PRINT GRID.VAULT.LIST$
40 V$ = GRID.VAULT.GET$("bh-persist-29")
50 IF LEN(V$) > 0 THEN PRINT "ALERT: persistence key" ELSE PRINT "clear"
60 END
