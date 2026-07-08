10 REM Encyclopedia: RESTORE
20 REM Where: GridBASIC program
30 REM Purpose: Reset DATA pointer
40 REM Action: Rewinds READ to first DATA
50 DATA 1
60 READ A
70 RESTORE
80 READ B
90 PRINT A; B
100 END
