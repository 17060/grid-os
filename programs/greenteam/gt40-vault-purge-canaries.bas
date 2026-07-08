10 REM gt40 -- vault purge-canaries
20 PRINT "=== GT40: Vault DevSecOps ==="
30 PRINT GRID.VAULT.LIST$
40 PRINT GRID.VAULT.GET$("redteam-canary")
50 PRINT "Remove lab canaries before prod"
60 END
