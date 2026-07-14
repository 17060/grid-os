10 REM g11 -- sine paint
20 PRINT "=== Sine Paint ==="
30 GRID.CLS
40 FOR X = 0 TO 79
50   LET Y = 12 + INT(SIN(X / 8) * 6)
60   GRID.PLOT X, Y, 2
70 NEXT X
80 PRINT "Wave complete."
90 END
