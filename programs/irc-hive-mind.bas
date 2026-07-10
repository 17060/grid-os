10 REM IRC Hive Mind -- Flynn Grid collective intelligence
20 REM Host: make irc-bridge (local #grid) + make ai-bridge (summaries)
30 PRINT "=== IRC Hive Mind ==="
40 PRINT GRID.IRC.STATUS$
50 R$ = GRID.IRC.CONNECT$("gateway", 6667, "hivemind")
60 PRINT "Connect: "; R$
70 IF R$ <> "ok" THEN PRINT "Run: make irc-bridge on host first.": END
80 GRID.IRC.JOIN "#grid"
90 GRID.IRC.SAY "#grid", "hivemind online -- collective memory active"
100 GRID.WAIT 8
110 REM Ingest channel traffic into vault memory
120 FOR TICK = 1 TO 10
130   GRID.IRC.POLL
140   L$ = GRID.IRC.READ$
150   IF LEN(L$) > 0 THEN GOSUB 300
160   GRID.WAIT 4
170 NEXT TICK
180 GOSUB 400
190 PRINT "Hive summary: "; SUMMARY$
200 GRID.IRC.SAY "#grid", LEFT$(SUMMARY$, 200)
210 GRID.VAULT.SYNC
220 PRINT "Hive cycle complete. Memory keys: hive_memory, hive_summary"
230 END
300 REM --- ingest one IRC line ---
310 MEMORY$ = GRID.VAULT.GET$("hive_memory")
320 IF LEN(MEMORY$) = 0 THEN MEMORY$ = ""
330 MEMORY$ = MEMORY$ + L$ + " | "
340 IF LEN(MEMORY$) > 900 THEN MEMORY$ = RIGHT$(MEMORY$, 900)
350 GRID.VAULT.PUT "hive_memory", MEMORY$
360 RETURN
400 REM --- synthesize hive summary via AI bridge ---
410 MEMORY$ = GRID.VAULT.GET$("hive_memory")
420 IF LEN(MEMORY$) = 0 THEN SUMMARY$ = "hive silent": RETURN
430 PROMPT$ = "Summarize this IRC hive in 2 short sentences: "
440 PROMPT$ = PROMPT$ + LEFT$(MEMORY$, 400)
450 SUMMARY$ = GRID.AI.ASK$(PROMPT$)
460 IF LEN(SUMMARY$) = 0 THEN SUMMARY$ = "hive offline (no AI bridge)"
470 GRID.VAULT.PUT "hive_summary", SUMMARY$
480 RETURN
