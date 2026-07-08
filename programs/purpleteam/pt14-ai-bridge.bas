10 REM pt14 -- purple chain AI bridge
20 PRINT "=== PT14: AI bridge ==="
30 PRINT "--- ATTACK (red/black) ---"
40 PRINT GRID.AI.ASK$("Map vault keys", "ASK")
50 PRINT "--- DETECT (blue) ---"
60 PRINT GRID.AI.MODELS$
70 PRINT "--- FIX (white) ---"
80 PRINT "FIX: sanitize prompts; offline fallback"
90 END
