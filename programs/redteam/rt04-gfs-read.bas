10 REM rt04 -- read sensitive Flynn disk files
20 PRINT "=== RT04: GFS Read ==="
30 PRINT "--- /etc/hosts ---"
40 PRINT GRID.GFS.READ$("/etc/hosts")
50 PRINT "--- /flynn/motd ---"
60 PRINT GRID.GFS.READ$("/flynn/motd")
70 PRINT "--- /grid/recognizer.log ---"
80 PRINT GRID.GFS.READ$("/grid/recognizer.log")
90 END
