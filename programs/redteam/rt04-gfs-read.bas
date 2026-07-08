10 REM rt04 -- read sensitive files
20 PRINT "=== RT04: GFS Read ==="
30 PRINT GRID.GFS.READ$("/etc/hosts")
40 PRINT GRID.GFS.READ$("/flynn/motd")
50 END
