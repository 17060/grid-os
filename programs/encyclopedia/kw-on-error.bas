10 REM Encyclopedia: ON ERROR GOTO
20 REM Where: GridBASIC program
30 REM Purpose: Error handler
40 REM Action: Jumps to line on runtime error
50 ON ERROR GOTO 900
60 X = 1/0
70 900 PRINT ERR$
80 END
90 END
