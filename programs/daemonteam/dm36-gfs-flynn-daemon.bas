10 REM dm36 -- gfs-flynn-daemon
20 PRINT "=== DM36: gfs-flynn-daemon ==="
30 PRINT GRID.GFS.LIST$("/flynn")
40 PRINT GRID.GFS.READ$("/flynn/motd")
50 END
