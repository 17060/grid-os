#ifndef BASIC_H
#define BASIC_H

#include <stddef.h>
#include <stdint.h>

/* GridBASIC — the advanced BASIC language of Grid OS 6.5.
 *
 * Programs may be run from source text (basic_run_source) or from a GFS
 * file (basic_run_file). The interpreter is in-kernel and runs in the
 * console context so programs can do I/O and call into the Grid. */

#define BASIC_LINE_MAX   256
#define BASIC_PROGRAM_LINES 512

/* Run a GridBASIC program from a NUL-terminated source string.
 * Returns 0 on normal END/STOP, non-zero on error. */
int basic_run_source(const char *source);

/* Run a GridBASIC program stored in GFS at `path`. */
int basic_run_file(const char *path);

/* Compile source to GRIDBC bytecode buffer. */
int basic_compile_source(const char *source, void *out, size_t cap, size_t *out_len);
int basic_compile_file(const char *path, const char *out_path);

/* Edit a program in the fullscreen GridBASIC IDE. The optional `path`
 * loads an existing program; pass NULL for a fresh buffer. */
int basic_ide(const char *path);

/* One-line hint shown on the IDE command row after boot (e.g. Flynn welcome). */
void basic_ide_set_boot_hint(const char *msg);

/* Lexer / value types shared with the IDE for syntax help. */
void basic_print_version(void);

#endif
