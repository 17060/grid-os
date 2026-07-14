10 REM g04 -- grid plot art
20 PRINT "=== GRID Plot Demo ==="
30 GRID.CLS
40 FOR X = 5 TO 75 STEP 5
50   GRID.PLOT X, 12, (X / 5) MOD 4
60 NEXT X
70 GRID.LINE 5, 12, 75, 12, 1
80 PRINT "Plotted on the Flynn grid overlay"
90 END
