/* Host-side test harness for the GridBASIC interpreter.
 * Compiles kernel/basic.c against stubs so we can feed programs on stdin
 * and observe printed output deterministically (no QEMU/serial involved). */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "basic.h"
#include "ai.h"
#include "btc.h"
#include "irc.h"

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
int net_resolve_host(const char *t, uint32_t *o) {
    if (t && t[0] == 'g' && t[1] == 'a') { if (o) *o = 0x0A000202u; return 0; }
    return net_parse_ip(t, o);
}
int net_ping(uint32_t ip)                    { (void)ip; return 0; }
int net_present(void)                         { return 1; }

/* ---- irc stubs ---- */
int irc_connect(const char *h, uint16_t p, const char *n) { (void)h;(void)p;(void)n; return 0; }
void irc_disconnect(void)                      { }
int irc_join(const char *c)                    { (void)c; return 0; }
int irc_part(const char *c)                    { (void)c; return 0; }
int irc_say(const char *t, const char *m)      { (void)t;(void)m; return 0; }
int irc_nick(const char *n)                    { (void)n; return 0; }
int irc_quit(const char *r)                    { (void)r; return 0; }
void irc_poll(void)                            { }
size_t irc_read(char *b, size_t cap)           { if(cap&&b)b[0]=0; return 0; }
void irc_status(char *b, size_t cap)           { if(cap)snprintf(b,cap,"disconnected"); }
int irc_is_connected(void)                     { return 0; }
int irc_session(const char *h, uint16_t p, const char *n, const char *c, uint32_t d) {
    (void)h;(void)p;(void)n;(void)c;(void)d; return 0;
}

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

/* ---- btc stubs (offline path only in host test) ---- */
int btc_call(const char *method, const char *params, char *out, size_t cap) {
    (void)method; (void)params;
    if (cap) { snprintf(out, cap, "btc-offline-host-test"); }
    return -1;
}
void btc_status(char *out, size_t cap) {
    if (cap) { snprintf(out, cap, "offline"); }
}
int btc_blockchain(char *out, size_t cap) { return btc_call("getblockchaininfo", "", out, cap); }
int btc_network(char *out, size_t cap) { return btc_call("getnetworkinfo", "", out, cap); }
int btc_wallet(char *out, size_t cap) { return btc_call("getwalletinfo", "", out, cap); }
int btc_balance(char *out, size_t cap) { return btc_call("getbalance", "", out, cap); }
int btc_address(const char *label, char *out, size_t cap) {
    (void)label; return btc_call("getnewaddress", "", out, cap);
}
int btc_send(const char *addr, const char *amount, char *out, size_t cap) {
    (void)addr; (void)amount; return btc_call("sendtoaddress", "", out, cap);
}
int btc_tx(const char *txid, char *out, size_t cap) {
    (void)txid; return btc_call("getrawtransaction", "", out, cap);
}
int btc_block(const char *hash_or_height, char *out, size_t cap) {
    (void)hash_or_height; return btc_call("getblock", "", out, cap);
}
int btc_help(char *out, size_t cap) { return btc_call("HELP", "", out, cap); }
int btc_stop(char *out, size_t cap) { return btc_call("stop", "", out, cap); }

int main(void) {
    static char buf[65536];
    size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
    buf[n] = '\0';
    return basic_run_source(buf);
}
