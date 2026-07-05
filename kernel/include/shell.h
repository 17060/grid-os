#ifndef SHELL_H
#define SHELL_H

/* Run one Flynn Grid shell command line (mutates `line` for argv parsing). */
void shell_dispatch_line(char *line);

/* GridBASIC IDE embeds the shell prompt; suppress re-entry for `basic`. */
void shell_set_in_basic_ide(int active);

#endif
