10 REM Encyclopedia: GRID.HTTP.GET$(h$,port,path$)
20 REM Where: GridBASIC GRID binding
30 REM Purpose: HTTP GET body
40 REM Action: Returns result of GRID.HTTP.GET$(h$,port,path$)
50 PRINT LEN(GRID.HTTP.GET$("gateway", 80, "/"))
60 END
