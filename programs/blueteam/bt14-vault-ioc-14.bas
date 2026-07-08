10 REM bt14 -- vault IOC bh-persist-24
20 PRINT "=== BT14: Vault IOC ==="
30 PRINT GRID.VAULT.LIST$
40 V$ = GRID.VAULT.GET$("bh-persist-24")
50 IF LEN(V$) > 0 THEN PRINT "ALERT: persistence key" ELSE PRINT "clear"
60 END
