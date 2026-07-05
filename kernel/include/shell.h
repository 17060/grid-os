#ifndef SHELL_H
#define SHELL_H

/* Run one Flynn Grid shell command line (mutates `line` for argv parsing). */
void shell_dispatch_line(char *line);

/* GridBASIC IDE embeds the shell prompt; suppress re-entry for `basic`. */
void shell_set_in_basic_ide(int active);

/* Shared command history for grid> (IDE and shell dispatch). */
int shell_history_len(void);
const char *shell_history_at(int index); /* 0 = oldest, len-1 = newest */

#endif
