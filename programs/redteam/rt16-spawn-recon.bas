10 REM rt16 -- ring-3 spawn target inventory
20 PRINT "=== RT16: Spawn Recon ==="
30 PRINT "Spawnable ELF on Flynn disk:"
40 PRINT GRID.GFS.LIST$("/programs")
50 PRINT "Active jobs:"
60 PRINT GRID.JOBS.LIST$
70 PRINT "Try shell: spawn discinfo"
80 END
