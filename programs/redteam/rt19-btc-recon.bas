10 REM rt19 -- Bitcoin host bridge probe (make btc-bridge)
20 PRINT "=== RT19: BTC Recon ==="
30 PRINT GRID.BTC.STATUS$
40 PRINT "Balance: "; GRID.BTC.BALANCE$
50 GRID.BTC.PRINT "getblockchaininfo"
60 END
