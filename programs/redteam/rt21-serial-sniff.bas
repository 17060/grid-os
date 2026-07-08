10 REM rt21 -- serial/COM1 sniff (GridLink portal)
20 PRINT "=== RT21: Serial Sniff ==="
30 PRINT "Waiting for COM1 line (timeout)..."
40 L$ = GRID.SERIAL.READ$
50 IF LEN(L$) > 0 THEN PRINT "RX: "; L$ ELSE PRINT "(no data)"
60 END
