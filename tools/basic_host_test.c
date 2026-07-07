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
#include "gfs.h"
#include "http.h"
#include "irc.h"
#include "security.h"
#include "storage.h"

/* ---- console stubs ---- */
void console_write(const char *t)      { fputs(t, stdout); fflush(stdout); }
void console_write_line(const char *t) { fputs(t, stdout); fputc('\n', stdout); fflush(stdout); }
void console_write_char(char c)        { fputc(c, stdout); fflush(stdout); }
void console_set_color(uint8_t c)      { (void)c; }
void console_clear(void)               { }
void console_reset_cursor(size_t x, size_t y) { (void)x; (void)y; }
void console_write_at(size_t x, size_t y, const char *t, uint8_t a) { (void)x;(void)y;(void)a; (void)t; }
int console_try_read_scancode(void)             { return -1; }
char console_read_char(void)           { return (char)getchar(); }
void console_read_line(char *b, size_t cap)   { if (fgets(b, (int)cap, stdin)) { b[strcspn(b,"\n")]=0; } else b[0]=0; }

/* ---- gfs stubs ---- */
int gfs_read_file(const char *p, void *o, size_t cap, size_t *len) { (void)p;(void)o;(void)cap; if(len)*len=0; return -1; }
int gfs_write_file(const char *p, const void *d, size_t n)         { (void)p;(void)d;(void)n; return 0; }
int gfs_list_paths(const char *prefix, char paths[][56], int max) {
    (void)prefix; (void)max; if (max > 0 && paths) paths[0][0] = '\0'; return 0;
}
int gfs_present(void)                          { return 1; }

/* ---- storage stubs ---- */
int storage_put(const char *k, const char *v)  { (void)k;(void)v; return 0; }
const char *storage_get(const char *k)         { (void)k; return "stub"; }
int storage_sync_disk(void)                    { return 0; }
int storage_list_keys(char *o, size_t n)       { if(n&&o)snprintf(o,n,"motd,seed"); return 0; }

/* ---- http stubs ---- */
int http_get_host(const char *h, uint16_t p, const char *path, char *o, size_t c) {
    (void)h;(void)p;(void)path; if(c&&o)snprintf(o,c,"HTTP-OK"); return 4;
}
int http_post_host(const char *h, uint16_t p, const char *path, const char *b, char *o, size_t c) {
    (void)h;(void)p;(void)path;(void)b; if(c&&o)snprintf(o,c,"POST-OK"); return 7;
}

/* ---- security stub ---- */
int security_has_capability(uint32_t cap)      { (void)cap; return 1; }

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

/* ---- log stub ---- */
void log_event(const char *m)                 { (void)m; }
int log_copy_tail(char *o, size_t n, int t)   { (void)t; if(n&&o)snprintf(o,n,"log-stub"); return 8; }

/* ---- sched / iso / program stubs ---- */
int sched_format_jobs(char *o, size_t n)       { if(n&&o)snprintf(o,n,"none"); return 4; }
int sched_kill(int id)                         { (void)id; return 0; }
int iso_format_list(char *o, size_t n)          { if(n&&o)snprintf(o,n,"empty"); return 5; }
int iso_spawn(const char *name)                { (void)name; return 1; }
int program_run_background(const char *n)      { (void)n; return 1; }
int net_format_status(char *o, size_t n)       { if(n&&o)snprintf(o,n,"online"); return 6; }
int net_format_ip(uint32_t ip, char *o, size_t n) {
    (void)ip; if(n&&o)snprintf(o,n,"10.0.2.2"); return 8;
}
void storage_export_serial(void)               { }
int storage_import_serial(void)                { return 0; }
const char *security_entity_name(void)         { return "User"; }
const grid_identity_t *security_current_identity(void) { return NULL; }

/* ---- program stub ---- */
int program_spawn_named(const char *n)        { (void)n; return 0; }

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

void disc_on_basic_run(void) {}
void disc_on_program_run(const char *n) { (void)n; }
int disc_level(void) { return 1; }
int disc_xp(void) { return 0; }
void disc_format_status(char *o, size_t c) { if (c && o) snprintf(o, c, "Disc L1 XP 0 User"); }
const char *disc_entity(void) { return "User"; }

void recognizer_start_patrol(void) {}
void recognizer_stop_patrol(void) {}
void recognizer_status(char *o, size_t c) { if (c && o) snprintf(o, c, "idle"); }

void speaker_beep(uint32_t f, uint32_t ms) { (void)f; (void)ms; }
void speaker_note(int n, uint32_t ms) { (void)n; (void)ms; }

void grid_plot(int x, int y, uint8_t c) { (void)x; (void)y; (void)c; }
void grid_line(int x0, int y0, int x1, int y1, uint8_t c) { (void)x0; (void)y0; (void)x1; (void)y1; (void)c; }
void grid_circle(int cx, int cy, int r, uint8_t c) { (void)cx; (void)cy; (void)r; (void)c; }

int gridlink_recv_file(void) { return -1; }
int gridlink_recv_package(void) { return -1; }
int gridlink_duel_ping(void) { return 0; }

int iso_evolve(int id) { (void)id; return 0; }

int main(void) {
    static char buf[65536];
    size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
    buf[n] = '\0';
    return basic_run_source(buf);
}
