10 REM Encyclopedia: ERR$
20 REM Where: GridBASIC expression
30 REM Purpose: Last error message
40 REM Action: Evaluates ERR$ and returns result
50 ON ERROR GOTO 100
60 X=1/0
70 100 PRINT ERR$
80 END
