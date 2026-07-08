10 REM Grid OS 4K HDMI demo — GridBASIC IDE
20 REM Run with: make run-4k
30 PRINT "=== Grid OS 4K Display Demo ==="
40 PRINT "Resolution: 3840 x 2160 (32-bit framebuffer)"
50 PRINT GRID.STATUS$
60 PRINT "WHOAMI: "; GRID.WHOAMI$
70 PRINT "Caps:   "; GRID.CAPS$
80 PRINT ""
90 PRINT "Try GRID.PLOT on the VGA grid overlay:"
100 GRID.PLOT 10, 12, 2
110 GRID.LINE 10, 12, 70, 12, 1
120 GRID.CIRCLE 40, 12, 8, 3
130 PRINT ""
140 PRINT "Esc :run to execute   Esc :help for IDE commands"
150 END
