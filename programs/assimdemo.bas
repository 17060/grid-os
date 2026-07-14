10 REM AssimBASIC — best features of the language universe
20 PRINT "=== AssimBASIC 7.2 ==="
30 PRINT GRID.STATUS$
40 X = 2
50 X += 3
60 PRINT "compound += "; X
70 PRINT "IIF "; IIF(X > 4, "yes", "no")
80 PRINT "TYPEOF "; TYPEOF$(X); " / "; TYPEOF$("grid")
90 PRINT "CLAMP "; CLAMP(99, 0, 10)
100 PRINT "REPLACE "; REPLACE$("hello world", "world", "grid")
110 PRINT "FIELD "; FIELD$("a,b,c", ",", 2)
120 PRINT "XOR "; (1 XOR 0); " "; (1 XOR 1)
130 MATCH X
140 WHEN 5
150   PRINT "MATCH when 5"
160 OTHERWISE
170   PRINT "MATCH otherwise"
180 END MATCH
190 UNLESS X < 0 THEN PRINT "UNLESS ok"
200 ASSERT X = 5
210 A = 1: B = 2: SWAP A, B
220 PRINT "SWAP "; A; " "; B
230 DIM N(3)
240 N(0) = 10: N(1) = 20: N(2) = 30: N(3) = 40
250 FOREACH I IN N
260   PRINT "foreach "; I; "="; N(I)
270 NEXT I
280 TRY
290   ASSERT 0
300 CATCH
310   PRINT "caught: "; ERR$
320 FINALLY
330   PRINT "finally"
340 END TRY
350 PRINT "AI models: "; GRID.AI.MODELS$
360 PRINT "AI run: "; GRID.AI.RUN$("ping")
370 PRINT "BTC: "; GRID.BTC.STATUS$
380 PRINT "IRC connected: "; GRID.IRC.CONNECTED
390 PRINT "Assim complete."
400 END
