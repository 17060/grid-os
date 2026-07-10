10 REM Encyclopedia: SHARED
20 REM Where: GridBASIC program
30 REM Purpose: Shared variable
40 REM Action: Marks module-level shared in SUB
50 SHARED C
60 SUB B()
70   C = 1
80 END SUB
90 CALL B()
100 PRINT C
110 END
