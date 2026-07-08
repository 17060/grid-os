10 REM Encyclopedia: EXIT FOR
20 REM Where: GridBASIC program
30 REM Purpose: Leave FOR early
40 REM Action: Jumps to statement after NEXT
50 FOR I = 1 TO 5
60 IF I = 3 THEN EXIT FOR
70 PRINT I
80 NEXT I
90 END
