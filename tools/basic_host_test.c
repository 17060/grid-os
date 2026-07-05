/* Host-side test harness for the GridBASIC interpreter.
 * Compiles kernel/basic.c against stubs so we can feed programs on stdin
 * and observe printed output deterministically (no QEMU/serial involved). */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "basic.h"
#include "ai.h"

/* ---- console stubs ---- */
void console_write(const char *t)      { fputs(t, stdout); fflush(stdout); }
void console_write_line(const char *t) { fputs(t, stdout); fputc('\n', stdout); fflush(stdout); }
void console_write_char(char c)        { fputc(c, stdout); fflush(stdout); }
void console_set_color(uint8_t c)      { (void)c; }
void console_clear(void)               { }
void console_reset_cursor(size_t x, size_t y) { (void)x; (void)y; }
char console_read_char(void)           { return (char)getchar(); }
void console_read_line(char *b, size_t cap)   { if (fgets(b, (int)cap, stdin)) { b[strcspn(b,"\n")]=0; } else b[0]=0; }

/* ---- gfs stubs ---- */
int gfs_read_file(const char *p, void *o, size_t cap, size_t *len) { (void)p;(void)o;(void)cap; if(len)*len=0; return -1; }
int gfs_write_file(const char *p, const void *d, size_t n)         { (void)p;(void)d;(void)n; return -1; }

/* ---- net stubs ---- */
int net_parse_ip(const char *t, uint32_t *o) { (void)t; if(o)*o=0; return -1; }
int net_ping(uint32_t ip)                    { (void)ip; return 1; }
int net_present(void)                         { return 1; }

/* ---- serial stubs ---- */
void serial_write(const char *t)             { (void)t; }
size_t serial_read_line(char *b, size_t cap, uint32_t lim) { (void)lim; if(cap)b[0]=0; return 0; }

/* ---- timer stub ---- */
static uint32_t g_ticks = 12345;
uint32_t timer_ticks(void)                    { return g_ticks++; }

/* ---- program stub ---- */
int program_spawn_named(const char *n)        { (void)n; return 0; }

/* ---- log stub ---- */
void log_event(const char *m)                 { (void)m; }

/* ---- ai stubs (offline path only in host test) ---- */
int ai_ask(const char *prompt, char *out, size_t cap) {
    (void)prompt;
    if (cap) { snprintf(out, cap, "offline-host-test"); }
    return 0;
}
int ai_explain(const char *line, char *out, size_t cap) {
    (void)line;
    if (cap) { snprintf(out, cap, "explain-stub"); }
    return 0;
}
int ai_fix(const char *code, char *out, size_t cap) {
    (void)code;
    if (cap) { snprintf(out, cap, "fix-stub"); }
    return 0;
}
int ai_complete(const char *code, char *out, size_t cap) {
    (void)code;
    if (cap) { snprintf(out, cap, "complete-stub"); }
    return 0;
}
int ai_models(char *out, size_t cap) {
    if (cap) { snprintf(out, cap, "stub"); }
    return 0;
}

int main(void) {
    static char buf[65536];
    size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
    buf[n] = '\0';
    return basic_run_source(buf);
}
