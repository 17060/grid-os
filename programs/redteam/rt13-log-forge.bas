10 REM rt13 -- forge audit log entry (repudiation demo)
20 PRINT "=== RT13: Log Forge ==="
30 GRID.LOG "REDTEAM: forged audit marker rt13"
40 PRINT "Injected audit event. Tail:"
50 PRINT GRID.LOG.TAIL$(4)
60 END
