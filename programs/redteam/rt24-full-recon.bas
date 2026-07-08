10 REM rt24 -- combined quick recon sweep
20 PRINT "=== RT24: Full Recon ==="
30 PRINT GRID.WHOAMI$; " caps="; GRID.CAPS$
40 PRINT GRID.NET.STATUS$
50 PRINT "ping gw="; GRID.PING("gateway"); " bridge="; GRID.PING("bridge")
60 PRINT "vault: "; GRID.VAULT.LIST$
70 PRINT "gfs /programs: "; GRID.GFS.LIST$("/programs")
80 PRINT "jobs: "; GRID.JOBS.LIST$
90 PRINT "audit:"
100 PRINT GRID.LOG.TAIL$(6)
110 END
