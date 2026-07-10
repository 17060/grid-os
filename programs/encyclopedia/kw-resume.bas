10 REM Encyclopedia: RESUME
20 REM Where: GridBASIC program
30 REM Purpose: Resume after error
40 REM Action: Continues after error handler
50 ON ERROR GOTO 100
60 X=1/0
70 100 PRINT "err"
80 RESUME NEXT
90 END
