10 REM pt04 -- purple chain sensitive file read
20 PRINT "=== PT04: sensitive file read ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT GRID.GFS.READ$("/etc/hosts")
50 PRINT "--- DETECT (blue) ---"
60 PRINT "IOC: unexpected hosts exfil len="; LEN(GRID.GFS.READ$("/etc/hosts"))
70 PRINT "--- FIX (white) ---"
80 PRINT "Policy: exfil only in authorized lab"
90 END
