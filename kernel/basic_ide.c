#include "basic.h"

#include "ai.h"
#include "console.h"
#include "gfs.h"
#include "grid.h"
#include "irc.h"
#include "pkg.h"
#include "security.h"
#include "shell.h"
#include "storage.h"

#include <stddef.h>
#include <stdint.h>

/* ===== GridBASIC IDE — fullscreen editor ==================================
 *
 * A line-oriented editor for GridBASIC programs. Lines are kept in an
 * in-memory buffer. Editing keys: arrows, Home/End, Enter (split line),
 * Backspace (merge up at col 0), Del. Escape opens the embedded Grid shell
 * prompt on the bottom row (grid>):
 *   grid> help          Flynn Grid shell commands
 *   grid> :run          run the current buffer
 *   grid> :save <name>  write to GFS /programs/<name>.bas
 *   grid> :load <name>  read from GFS
 *   grid> :new          clear the buffer
 *   grid> :list         print the buffer (paused)
 *   grid> :mod run <n>   run installed IDE module
 *   grid> :mod load <n> load module source into editor
 *   grid> :mods         list IDE modules (pkg mods)
 *   grid> :help         IDE editing help
 *   grid> :ai ask ...   AI prompt (host bridge or offline help)
 *   grid> :ai explain   explain current line
 *   grid> :ai complete  suggest completion for buffer
 *   grid> btc info      Bitcoin node (host bridge via make btc-bridge)
 *   grid> :btc balance  same as btc balance (fullscreen shell output)
 * ========================================================================== */

#define IDE_MAX_LINES  2048
#define IDE_LINE_LEN   200
#define IDE_HEADER_ROW 0
#define IDE_BODY_TOP   2
#define IDE_BODY_BOT   (CONSOLE_ROWS - 3)
#define IDE_FOOTER_ROW (CONSOLE_ROWS - 2)
#define IDE_CMD_ROW    (CONSOLE_ROWS - 1)
#define IDE_SHELL_PROMPT "grid> "
#define IDE_SHELL_PROMPT_LEN 6

typedef struct {
    char lines[IDE_MAX_LINES][IDE_LINE_LEN];
    int n;
    int row;        /* cursor line */
    int col;        /* cursor column within line */
    int top;        /* first visible line */
    char path[64];  /* current file path or "" */
    int dirty;
    char status[96];     /* pending status message for the shell row */
    uint8_t status_attr;
} ide_t;

static void run_shell_line(ide_t *e, const char *line);

static ide_t ide;
static char g_boot_hint[96];
/* One shared IDE source buffer (load/save/run/compile). Separate from basic.c g_src_buf for :run. */
static char g_ide_io_buf[BASIC_SRC_MAX];
static char g_ide_bc_buf[16384];

static size_t slen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static void scopy(char *d, size_t cap, const char *s) {
    size_t n = 0; while (s[n] && n + 1 < cap) { d[n] = s[n]; n++; } d[n] = '\0';
}

void basic_ide_set_boot_hint(const char *msg) {
    scopy(g_boot_hint, sizeof(g_boot_hint), msg ? msg : "");
}
static int sequal(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; } return *a == *b;
}
static int starts_with(const char *s, const char *p) {
    while (*p) { if (*s++ != *p++) return 0; } return 1;
}

static void insert_char_at(ide_t *e, int row, int col, char c) {
    if (row < 0 || row >= e->n) return;
    char *line = e->lines[row];
    size_t len = slen(line);
    if (len + 1 >= IDE_LINE_LEN) return;
    for (size_t i = len + 1; i > (size_t)col; --i) line[i] = line[i - 1];
    line[col] = c;
}

static void split_line(ide_t *e, int row, int col) {
    if (e->n >= IDE_MAX_LINES) return;
    if (row < 0 || row >= e->n) return;
    /* shift lines down */
    for (int i = e->n; i > row + 1; --i) scopy(e->lines[i], IDE_LINE_LEN, e->lines[i - 1]);
    e->n++;
    char *cur = e->lines[row];
    char *nl  = e->lines[row + 1];
    size_t len = slen(cur);
    /* tail goes to new line */
    size_t k = 0;
    for (size_t i = (size_t)col; i < len && k + 1 < IDE_LINE_LEN; ++i) nl[k++] = cur[i];
    nl[k] = '\0';
    cur[col] = '\0';
}

static void merge_up(ide_t *e, int row) {
    /* merge line `row` into line row-1 */
    if (row <= 0 || row >= e->n) return;
    char *prev = e->lines[row - 1];
    char *cur  = e->lines[row];
    size_t plen = slen(prev);
    size_t clen = slen(cur);
    if (plen + clen + 1 >= IDE_LINE_LEN) {
        /* join what fits; keep cursor at join */
        size_t room = IDE_LINE_LEN - 2 - plen;
        for (size_t i = 0; i <= room && i < clen; ++i) prev[plen + i] = cur[i];
        prev[plen + (room < clen ? room : clen)] = '\0';
    } else {
        scopy(prev + plen, IDE_LINE_LEN - plen, cur);
    }
    for (int i = row; i < e->n - 1; ++i) scopy(e->lines[i], IDE_LINE_LEN, e->lines[i + 1]);
    e->n--;
    e->row = row - 1;
    e->col = (int)plen;
}

static void delete_char(ide_t *e, int row, int col) {
    if (row < 0 || row >= e->n) return;
    char *line = e->lines[row];
    size_t len = slen(line);
    if ((size_t)col >= len) {
        /* merge next line up into this one */
        if (row + 1 < e->n) {
            char *next = e->lines[row + 1];
            size_t nlen = slen(next);
            if (len + nlen + 1 < IDE_LINE_LEN) {
                scopy(line + len, IDE_LINE_LEN - len, next);
            }
            for (int i = row + 1; i < e->n - 1; ++i) scopy(e->lines[i], IDE_LINE_LEN, e->lines[i + 1]);
            e->n--;
        }
        return;
    }
    for (size_t i = (size_t)col; i < len; ++i) line[i] = line[i + 1];
}

static void clamp_cursor(ide_t *e) {
    if (e->n == 0) { e->n = 1; e->lines[0][0] = '\0'; }
    if (e->row < 0) e->row = 0;
    if (e->row >= e->n) e->row = e->n - 1;
    size_t len = slen(e->lines[e->row]);
    if (e->col < 0) e->col = 0;
    if ((size_t)e->col > len) e->col = (int)len;
}

static void ensure_visible(ide_t *e) {
    if (e->row < e->top) e->top = e->row;
    if (e->row >= e->top + (IDE_BODY_BOT - IDE_BODY_TOP + 1)) {
        e->top = e->row - (IDE_BODY_BOT - IDE_BODY_TOP);
    }
    if (e->top < 0) e->top = 0;
}

static int is_kw_char(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '$' || c == '.' || c == '%';
}

static int is_basic_keyword(const char *w) {
    static const char *kws[] = {
        "PRINT", "LET", "IF", "THEN", "ELSE", "FOR", "TO", "STEP", "NEXT",
        "WHILE", "WEND", "REPEAT", "UNTIL", "GOTO", "GOSUB", "RETURN",
        "INPUT", "LINE", "DIM", "CONST", "DATA", "READ", "RESTORE", "RANDOMIZE",
        "SELECT", "CASE", "EXIT", "CONTINUE", "ELSEIF", "ON", "ERROR", "RESUME",
        "OPTION", "BASE", "DEF", "FN", "SUB", "FUNCTION", "LOCAL", "SHARED", "CALL",
        "REM", "END", "STOP",
        "AND", "OR", "NOT", "MOD", "DIV", 0
    };
    char upper[48];
    size_t wi = 0;
    while (w[wi] && wi + 1 < sizeof(upper)) {
        char c = w[wi];
        upper[wi] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
        wi++;
    }
    upper[wi] = '\0';
    for (int i = 0; kws[i]; ++i) {
        const char *k = kws[i];
        const char *p = upper;
        while (*k && *p && *k == *p) {
            k++;
            p++;
        }
        if (*k == 0 && !is_kw_char(*p)) {
            return 1;
        }
    }
    return 0;
}

static const char *find_comment(const char *line) {
    const char *p = line;
    while (*p == ' ') {
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        p++;
    }
    while (*p == ' ') {
        p++;
    }
    if (p[0] == 'R' && p[1] == 'E' && p[2] == 'M' &&
        (p[3] == ' ' || p[3] == '\t' || p[3] == '\0')) {
        return p;
    }
    if (*p == '\'') {
        return p;
    }
    return 0;
}

static void draw_code_line(size_t y, const char *line) {
    const char *comment = find_comment(line);
    if (comment && comment == line) {
        console_write_at(6, y, line, GRID_COL_DIM);
        return;
    }

    int x = 6;
    const char *p = line;
    while (*p && x < CONSOLE_COLS) {
        if (comment && p >= comment) {
            console_write_at(x, y, p, GRID_COL_DIM);
            return;
        }
        if (*p == ' ' || *p == '\t') {
            console_write_at(x++, y, " ", GRID_COL_DEFAULT);
            p++;
            continue;
        }
        if (*p == '"') {
            char buf[IDE_LINE_LEN];
            size_t i = 0;
            buf[i++] = *p++;
            while (*p && *p != '"' && i + 1 < sizeof(buf)) {
                buf[i++] = *p++;
            }
            if (*p == '"') {
                buf[i++] = *p++;
            }
            buf[i] = '\0';
            console_write_at(x, y, buf, GRID_COL_WARN);
            x += (int)slen(buf);
            continue;
        }
        if (*p == '?') {
            console_write_at(x++, y, "?", GRID_COL_TITLE);
            p++;
            continue;
        }
        if (!is_kw_char(*p)) {
            char cbuf[2] = { *p, '\0' };
            console_write_at(x++, y, cbuf, GRID_COL_DEFAULT);
            p++;
            continue;
        }
        char word[48];
        size_t wi = 0;
        while (is_kw_char(*p) && wi + 1 < sizeof(word)) {
            word[wi++] = *p++;
        }
        word[wi] = '\0';
        uint8_t attr = GRID_COL_DEFAULT;
        if (starts_with(word, "GRID.")) {
            attr = GRID_COL_OK;
        } else if (is_basic_keyword(word)) {
            attr = GRID_COL_TITLE;
        }
        console_write_at(x, y, word, attr);
        x += (int)wi;
    }
}

static void draw_header(ide_t *e) {
    console_fill_row(IDE_HEADER_ROW, ' ', GRID_COL_TITLE);
    char hdr[80];
    const char *fn = e->path[0] ? e->path : "<untitled>";
    int n = 0;
    const char *t = " GridBASIC IDE — ";
    while (*t && n < (int)sizeof(hdr) - 1) hdr[n++] = *t++;
    while (*fn && n < (int)sizeof(hdr) - 1) hdr[n++] = *fn++;
    hdr[n] = '\0';
    console_write_at(0, IDE_HEADER_ROW, hdr, GRID_COL_TITLE);
    char tag[24];
    const char *s = e->dirty ? " * " : "   ";
    console_write_at(CONSOLE_COLS - 4, IDE_HEADER_ROW, s, GRID_COL_TITLE);
    (void)tag;
}

static void draw_body(ide_t *e) {
    int visible = IDE_BODY_BOT - IDE_BODY_TOP + 1;
    for (int i = 0; i < visible; ++i) {
        int ln = e->top + i;
        size_t y = (size_t)(IDE_BODY_TOP + i);
        if (ln >= e->n) {
            console_fill_row(y, ' ', GRID_COL_DEFAULT);
            console_write_at(0, y, "~", GRID_COL_DIM);
            continue;
        }
        console_fill_row(y, ' ', GRID_COL_DEFAULT);
        char num[12];
        /* line number prefix */
        int nn = 0;
        char tmp[12]; int t = 0; int v = ln + 1;
        if (v == 0) tmp[t++] = '0';
        while (v > 0) { tmp[t++] = (char)('0' + (v % 10)); v /= 10; }
        char nb[12]; int p = 0;
        while (t > 0) nb[p++] = tmp[--t];
        nb[p] = '\0';
        while (nb[nn]) num[nn] = nb[nn], nn++;
        num[nn] = '\0';
        console_write_at(0, y, num, GRID_COL_DIM);
        draw_code_line(y, e->lines[ln]);
    }
}

static void draw_footer(ide_t *e) {
    console_fill_row(IDE_FOOTER_ROW, ' ', GRID_COL_MENU);
    const char *bar = " ESC: grid> command   Arrows: move   Enter: new line   Del/Bksp: edit ";
    console_write_at(0, IDE_FOOTER_ROW, bar, GRID_COL_MENU);
    if (irc_is_connected()) {
        char st[48];
        irc_status(st, sizeof(st));
        console_write_at(0, IDE_FOOTER_ROW, st, GRID_COL_OK);
    }
    char st[64];
    int n = 0;
    const char *s = " Ln ";
    while (*s && n < (int)sizeof(st) - 1) st[n++] = *s++;
    int v = e->row + 1; char tmp[12]; int t = 0;
    if (v == 0) tmp[t++] = '0';
    while (v > 0) { tmp[t++] = (char)('0' + (v % 10)); v /= 10; }
    while (t > 0 && n < (int)sizeof(st) - 1) st[n++] = tmp[--t];
    const char *s2 = " Col ";
    while (*s2 && n < (int)sizeof(st) - 1) st[n++] = *s2++;
    v = e->col + 1; t = 0;
    if (v == 0) tmp[t++] = '0';
    while (v > 0) { tmp[t++] = (char)('0' + (v % 10)); v /= 10; }
    while (t > 0 && n < (int)sizeof(st) - 1) st[n++] = tmp[--t];
    st[n] = '\0';
    console_write_at(CONSOLE_COLS - 16, IDE_FOOTER_ROW, st, GRID_COL_MENU);
}

static void draw_shell_row(ide_t *e) {
    console_fill_row(IDE_CMD_ROW, ' ', GRID_COL_DEFAULT);
    if (e->status[0]) {
        console_write_at(0, IDE_CMD_ROW, e->status, e->status_attr);
        return;
    }
    console_write_at(0, IDE_CMD_ROW, IDE_SHELL_PROMPT, GRID_COL_DIM);
    console_write_at(IDE_SHELL_PROMPT_LEN, IDE_CMD_ROW, "Esc", GRID_COL_DIM);
}

static void ide_redraw(ide_t *e) {
    console_clear();
    draw_header(e);
    draw_body(e);
    draw_footer(e);
    draw_shell_row(e);
    console_reset_cursor(6 + (size_t)e->col, (size_t)(IDE_BODY_TOP + (e->row - e->top)));
}

/* ---- colon command bar ---- */

static void ide_status(ide_t *e, const char *msg, uint8_t attr) {
    scopy(e->status, sizeof(e->status), msg);
    e->status_attr = attr;
    console_fill_row(IDE_CMD_ROW, ' ', attr);
    console_write_at(0, IDE_CMD_ROW, msg, attr);
}

static void redraw_cmd_row(const char *prompt, size_t prompt_len, const char *out) {
    console_fill_row(IDE_CMD_ROW, ' ', GRID_COL_DEFAULT);
    console_write_at(0, IDE_CMD_ROW, prompt, GRID_COL_OK);
    console_write_at((int)prompt_len, IDE_CMD_ROW, out, GRID_COL_DEFAULT);
}

static void read_cmd(char *out, size_t cap, const char *prompt) {
    size_t prompt_len = slen(prompt);
    redraw_cmd_row(prompt, prompt_len, "");
    size_t len = 0;
    int col = (int)prompt_len;
    int history_browse = shell_history_len();
    for (;;) {
        int k = console_read_key();
        if (k >= 0x100) {
            if (k == CONSOLE_SC_UP || k == CONSOLE_SC_DOWN) {
                int count = shell_history_len();
                if (count > 0) {
                    if (k == CONSOLE_SC_UP) {
                        if (history_browse > 0) {
                            history_browse--;
                        }
                    } else if (history_browse < count) {
                        history_browse++;
                    }
                    if (history_browse < count) {
                        const char *h = shell_history_at(history_browse);
                        len = 0;
                        while (h[len] && len + 1 < cap) {
                            out[len] = h[len];
                            len++;
                        }
                        out[len] = '\0';
                    } else {
                        out[0] = '\0';
                        len = 0;
                    }
                    col = (int)prompt_len + (int)len;
                    redraw_cmd_row(prompt, prompt_len, out);
                }
                continue;
            }
            if (k == CONSOLE_SC_LEFT && col > (int)prompt_len) { col--; continue; }
            if (k == CONSOLE_SC_RIGHT && (size_t)col < prompt_len + len) { col++; continue; }
            if (k == CONSOLE_SC_HOME) { col = (int)prompt_len; continue; }
            if (k == CONSOLE_SC_END) { col = (int)prompt_len + (int)len; continue; }
            if (k == CONSOLE_SC_DEL) {
                if ((size_t)col < prompt_len + len) {
                    size_t idx = (size_t)col - prompt_len;
                    for (size_t i = idx; i < len; ++i) out[i] = out[i + 1];
                    len--; out[len] = '\0';
                    history_browse = shell_history_len();
                    redraw_cmd_row(prompt, prompt_len, out);
                }
                continue;
            }
            continue;
        }
        char c = (char)k;
        if (c == '\n') { out[len] = '\0'; return; }
        if (c == 27) { out[0] = '\0'; return; }   /* Esc cancels */
        if (c == '\b') {
            if (col > (int)prompt_len && len > 0) {
                size_t idx = (size_t)col - prompt_len - 1;
                for (size_t i = idx; i < len - 1; ++i) out[i] = out[i + 1];
                len--; out[len] = '\0'; col--;
                history_browse = shell_history_len();
                redraw_cmd_row(prompt, prompt_len, out);
            }
            continue;
        }
        if (c >= 32 && c < 127 && len + 1 < cap) {
            size_t idx = (size_t)col - prompt_len;
            for (size_t i = len + 1; i > idx; --i) out[i] = out[i - 1];
            out[idx] = c; len++; col++;
            history_browse = shell_history_len();
            redraw_cmd_row(prompt, prompt_len, out);
        }
    }
}

/* Serialize buffer into a single source string. Returns 0, or -1 if truncated. */
static int serialize(ide_t *e, char *out, size_t cap) {
    size_t p = 0;
    for (int i = 0; i < e->n; ++i) {
        const char *l = e->lines[i];
        size_t len = slen(l);
        for (size_t j = 0; j < len; ++j) {
            if (p + 2 >= cap) {
                out[p < cap ? p : cap - 1] = '\0';
                return -1;
            }
            out[p++] = l[j];
        }
        if (p + 1 >= cap) {
            out[p < cap ? p : cap - 1] = '\0';
            return -1;
        }
        out[p++] = '\n';
    }
    out[p] = '\0';
    return 0;
}

static size_t source_bytes(ide_t *e) {
    size_t total = 0;
    for (int i = 0; i < e->n; ++i) {
        total += slen(e->lines[i]) + 1;
    }
    return total;
}

static int source_over_limit(ide_t *e) {
    return source_bytes(e) >= BASIC_SRC_MAX;
}

static void run_finish(ide_t *e, int rc) {
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key to return to IDE ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
    if (rc != 0) {
        ide_status(e, "run failed — see error above", GRID_COL_ERROR);
    } else {
        ide_status(e, "run OK", GRID_COL_OK);
    }
    ide_redraw(e);
}

static void make_path(const char *name, char *out, size_t cap) {
    if (starts_with(name, "/")) { scopy(out, cap, name); return; }
    size_t p = 0;
    const char *pre = "/programs/";
    while (*pre && p + 1 < cap) out[p++] = *pre++;
    const char *s = name;
    while (*s && p + 1 < cap) out[p++] = *s++;
    /* ensure .bas suffix unless .grid requested */
    if (p >= 5 && out[p - 5] == '.' && out[p - 4] == 'g' && out[p - 3] == 'r' &&
        out[p - 2] == 'i' && out[p - 1] == 'd') {
        out[p] = '\0';
        return;
    }
    if (!(p >= 4 && out[p-4]=='.' && out[p-3]=='b' && out[p-2]=='a' && out[p-1]=='s')) {
        const char *suf = ".bas";
        while (*suf && p + 1 < cap) out[p++] = *suf++;
    }
    out[p] = '\0';
}

static int path_ends_grid(const char *path) {
    size_t n = slen(path);
    return n >= 5 && path[n - 5] == '.' && path[n - 4] == 'g' &&
           path[n - 3] == 'r' && path[n - 2] == 'i' && path[n - 1] == 'd';
}

static void run_buffer(ide_t *e, const char *file_path) {
    int rc = 0;
    if (!(file_path && file_path[0]) &&
        !(e->path[0] && path_ends_grid(e->path)) &&
        source_over_limit(e)) {
        ide_status(e, "run blocked: program exceeds 65535 bytes", GRID_COL_ERROR);
        return;
    }
    console_clear();
    console_set_color(GRID_COL_TITLE);
    console_write_line("=== GridBASIC run ===");
    console_set_color(GRID_COL_DEFAULT);
    if (file_path && file_path[0]) {
        char path[GFS_PATH_MAX];
        if (starts_with(file_path, "/")) {
            scopy(path, sizeof(path), file_path);
        } else {
            make_path(file_path, path, sizeof(path));
        }
        rc = basic_run_file(path);
    } else if (e->path[0] && path_ends_grid(e->path)) {
        rc = basic_run_file(e->path);
    } else {
        if (serialize(e, g_ide_io_buf, sizeof(g_ide_io_buf)) != 0) {
            ide_status(e, "run blocked: program exceeds 65535 bytes", GRID_COL_ERROR);
            return;
        }
        rc = basic_run_source(g_ide_io_buf);
    }
    run_finish(e, rc);
}

static void cmd_save(ide_t *e, const char *name) {
    if (!name[0]) { ide_status(e, "usage: save <name>", GRID_COL_ERROR); return; }
    if (source_over_limit(e)) {
        ide_status(e, "save blocked: program exceeds 65535 bytes", GRID_COL_ERROR);
        return;
    }
    char path[80]; make_path(name, path, sizeof(path));
    if (serialize(e, g_ide_io_buf, sizeof(g_ide_io_buf)) != 0) {
        ide_status(e, "save blocked: program exceeds 65535 bytes", GRID_COL_ERROR);
        return;
    }
    if (gfs_write_file(path, g_ide_io_buf, slen(g_ide_io_buf)) != 0) {
        ide_status(e, "save failed (GFS write denied)", GRID_COL_ERROR); return;
    }
    scopy(e->path, sizeof(e->path), path);
    e->dirty = 0;
    char msg[80]; scopy(msg, sizeof(msg), "saved "); scopy(msg + 6, sizeof(msg) - 6, path);
    ide_status(e, msg, GRID_COL_OK);
}

static void cmd_compile(ide_t *e, const char *name) {
    if (!name[0]) {
        ide_status(e, "usage: compile <name>", GRID_COL_ERROR);
        return;
    }
    char path[80];
    if (starts_with(name, "/")) {
        scopy(path, sizeof(path), name);
    } else {
        size_t p = 0;
        const char *pre = "/programs/";
        while (*pre && p + 1 < sizeof(path)) {
            path[p++] = *pre++;
        }
        const char *s = name;
        while (*s && p + 1 < sizeof(path)) {
            path[p++] = *s++;
        }
        if (!(p >= 6 && path[p - 6] == '.' && path[p - 5] == 'g' &&
              path[p - 4] == 'r' && path[p - 3] == 'i' && path[p - 2] == 'd')) {
            const char *suf = ".grid";
            while (*suf && p + 1 < sizeof(path)) {
                path[p++] = *suf++;
            }
        }
        path[p] = '\0';
    }
    if (source_over_limit(e)) {
        ide_status(e, "compile blocked: program exceeds 65535 bytes", GRID_COL_ERROR);
        return;
    }
    size_t blen = 0;
    if (serialize(e, g_ide_io_buf, sizeof(g_ide_io_buf)) != 0) {
        ide_status(e, "compile blocked: program exceeds 65535 bytes", GRID_COL_ERROR);
        return;
    }
    if (basic_compile_source(g_ide_io_buf, g_ide_bc_buf, sizeof(g_ide_bc_buf), &blen) != 0) {
        ide_status(e, "compile failed", GRID_COL_ERROR);
        return;
    }
    if (gfs_write_file(path, g_ide_bc_buf, blen) != 0) {
        ide_status(e, "compile write failed", GRID_COL_ERROR);
        return;
    }
    char msg[80];
    scopy(msg, sizeof(msg), "compiled ");
    scopy(msg + 9, sizeof(msg) - 9, path);
    ide_status(e, msg, GRID_COL_OK);
}

static void cmd_tutorial_steps(ide_t *e) {
    static const char *hints[] = {
        "=== GridBASIC interactive tutorial ===",
        "Step 1: PRINT shows text — try :run on a PRINT line",
        "Step 2: Esc grid> tutorial runs /programs/tutorial.bas",
        "Step 3: :load subdemo — SUB/FUNCTION/CALL demo",
        "Step 4: :compile hello — writes bytecode .grid on Flynn disk",
        "Step 5: GRID.PLOT/LINE/CIRCLE draw on the VGA grid",
        "End of line — type :samples for more programs"
    };
    for (int i = 0; i < 7; ++i) {
        console_clear();
        console_set_color(GRID_COL_TITLE);
        console_write_line(hints[i]);
        console_set_color(GRID_COL_DIM);
        console_write_line("--- press any key ---");
        console_set_color(GRID_COL_DEFAULT);
        (void)console_read_key();
    }
    ide_redraw(e);
    ide_status(e, "Tutorial complete — :load tutorial", GRID_COL_OK);
}

static int parse_source_into(ide_t *e, const char *path, const char *src, size_t got) {
    e->n = 0;
    e->row = 0;
    e->col = 0;
    e->top = 0;
    size_t i = 0;
    int row = 0;
    size_t col = 0;
    e->lines[0][0] = '\0';
    while (src[i] && row < IDE_MAX_LINES) {
        char c = src[i++];
        if (c == '\n') {
            row++;
            col = 0;
            if (row < IDE_MAX_LINES) {
                e->lines[row][0] = '\0';
            }
        } else if (c == '\r') {
        } else if (col + 1 < IDE_LINE_LEN) {
            e->lines[row][col++] = c;
            e->lines[row][col] = '\0';
        }
    }
    if (row >= IDE_MAX_LINES) {
        row = IDE_MAX_LINES - 1;
    }
    e->n = row + 1;
    if (e->n == 0) {
        e->n = 1;
        e->lines[0][0] = '\0';
    }
    scopy(e->path, sizeof(e->path), path);
    e->dirty = 0;
    if (got >= BASIC_SRC_MAX - 1 || src[i]) {
        return 2;
    }
    return 0;
}

static int load_path_into(ide_t *e, const char *path) {
    size_t got = 0;
    if (gfs_read_file(path, g_ide_io_buf, sizeof(g_ide_io_buf) - 1, &got) != 0) {
        return -1;
    }
    g_ide_io_buf[got] = '\0';
    return parse_source_into(e, path, g_ide_io_buf, got);
}

int basic_ide_load_module(const char *path) {
    if (!path || !path[0]) {
        return -1;
    }
    if (load_path_into(&ide, path) != 0) {
        return -1;
    }
    ide_redraw(&ide);
    return 0;
}

static void cmd_mod_run(ide_t *e, const char *name) {
    if (!name[0]) {
        ide_status(e, "usage: mod run <name>", GRID_COL_ERROR);
        return;
    }
    console_clear();
    if (pkg_run_module(name) != 0) {
        ide_status(e, "module not found", GRID_COL_ERROR);
        return;
    }
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key to return to GridBASIC ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
    ide_redraw(e);
}

static void cmd_mod_load(ide_t *e, const char *name) {
    char path[GFS_PATH_MAX];
    if (!name[0]) {
        ide_status(e, "usage: mod load <name>", GRID_COL_ERROR);
        return;
    }
    if (pkg_find_module(name, path, sizeof(path), 0, 0) != 0) {
        ide_status(e, "module not found", GRID_COL_ERROR);
        return;
    }
    int lrc = load_path_into(e, path);
    if (lrc < 0) {
        ide_status(e, "module load failed", GRID_COL_ERROR);
        return;
    }
    char msg[80];
    scopy(msg, sizeof(msg), "mod ");
    scopy(msg + 4, sizeof(msg) - 4, name);
    if (lrc == 2) {
        scopy(msg + slen(msg), sizeof(msg) - slen(msg), " (truncated 65536)");
        ide_status(e, msg, GRID_COL_WARN);
        return;
    }
    ide_status(e, msg, GRID_COL_OK);
}

static void cmd_mods_list(ide_t *e, const char *category) {
    char line[80];
    if (category && category[0]) {
        scopy(line, sizeof(line), "pkg mods ");
        scopy(line + 9, sizeof(line) - 9, category);
    } else {
        scopy(line, sizeof(line), "pkg mods");
    }
    run_shell_line(e, line);
}

static void cmd_pkg(ide_t *e, const char *args) {
    char buf[96];
    scopy(buf, sizeof(buf), args ? args : "");
    if (!buf[0] || sequal(buf, "list")) {
        run_shell_line(e, "pkg list");
        return;
    }
    if (sequal(buf, "mods")) {
        run_shell_line(e, "pkg mods");
        return;
    }
    if (starts_with(buf, "mods ")) {
        char line[80];
        scopy(line, sizeof(line), "pkg mods ");
        scopy(line + 9, sizeof(line) - 9, buf + 5);
        run_shell_line(e, line);
        return;
    }
    if (starts_with(buf, "run ")) {
        cmd_mod_run(e, buf + 4);
        return;
    }
    if (starts_with(buf, "load ")) {
        cmd_mod_load(e, buf + 5);
        return;
    }
    if (starts_with(buf, "info ")) {
        char line[80];
        scopy(line, sizeof(line), "pkg info ");
        scopy(line + 9, sizeof(line) - 9, buf + 5);
        run_shell_line(e, line);
        return;
    }
    ide_status(e, ":pkg list|mods|run|load|info", GRID_COL_WARN);
}

static void cmd_find(ide_t *e, const char *needle) {
    if (!needle[0]) {
        ide_status(e, "usage: find <text>", GRID_COL_ERROR);
        return;
    }
    char upper[48];
    size_t ni = 0;
    while (needle[ni] && ni + 1 < sizeof(upper)) {
        char c = needle[ni];
        upper[ni] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
        ni++;
    }
    upper[ni] = '\0';
    for (int row = e->row; row < e->n; ++row) {
        char line_upper[IDE_LINE_LEN];
        size_t li = 0;
        const char *line = e->lines[row];
        while (line[li] && li + 1 < sizeof(line_upper)) {
            char c = line[li];
            line_upper[li] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
            li++;
        }
        line_upper[li] = '\0';
        size_t pos = 0;
        while (line_upper[pos]) {
            size_t j = 0;
            while (upper[j] && line_upper[pos + j] == upper[j]) {
                j++;
            }
            if (upper[j] == '\0') {
                e->row = row;
                e->col = (int)pos;
                clamp_cursor(e);
                ensure_visible(e);
                ide_redraw(e);
                char msg[64];
                scopy(msg, sizeof(msg), "found L");
                char num[12];
                int v = row + 1;
                int t = 0;
                char tmp[12];
                if (v == 0) {
                    tmp[t++] = '0';
                }
                while (v > 0) {
                    tmp[t++] = (char)('0' + (v % 10));
                    v /= 10;
                }
                while (t > 0 && slen(msg) + 1 < sizeof(msg)) {
                    num[0] = tmp[--t];
                    num[1] = '\0';
                    scopy(msg + slen(msg), sizeof(msg) - slen(msg), num);
                }
                ide_status(e, msg, GRID_COL_OK);
                return;
            }
            pos++;
        }
    }
    ide_status(e, "find: no match", GRID_COL_WARN);
}

static void cmd_goto(ide_t *e, const char *num_text) {
    if (!num_text[0]) {
        ide_status(e, "usage: goto <line>", GRID_COL_ERROR);
        return;
    }
    int line = 0;
    for (size_t i = 0; num_text[i]; ++i) {
        if (num_text[i] < '0' || num_text[i] > '9') {
            ide_status(e, "usage: goto <line>", GRID_COL_ERROR);
            return;
        }
        line = line * 10 + (num_text[i] - '0');
    }
    if (line < 1 || line > e->n) {
        ide_status(e, "goto: line out of range", GRID_COL_ERROR);
        return;
    }
    e->row = line - 1;
    e->col = 0;
    clamp_cursor(e);
    ensure_visible(e);
    ide_redraw(e);
    char msg[32];
    scopy(msg, sizeof(msg), "goto line ");
    scopy(msg + 10, sizeof(msg) - 10, num_text);
    ide_status(e, msg, GRID_COL_OK);
}

static void cmd_load(ide_t *e, const char *name) {
    if (!name[0]) { ide_status(e, "usage: load <name>", GRID_COL_ERROR); return; }
    char path[80]; make_path(name, path, sizeof(path));
    int lrc = load_path_into(e, path);
    if (lrc < 0) {
        ide_status(e, "load failed (not found)", GRID_COL_ERROR);
        return;
    }
    char msg[80]; scopy(msg, sizeof(msg), "loaded ");
    scopy(msg + 7, sizeof(msg) - 7, path);
    if (lrc == 2) {
        scopy(msg + slen(msg), sizeof(msg) - slen(msg), " (truncated 65536)");
        ide_status(e, msg, GRID_COL_WARN);
        return;
    }
    ide_status(e, msg, GRID_COL_OK);
}

static void cmd_list(ide_t *e) {
    console_clear();
    console_set_color(GRID_COL_TITLE);
    console_write_line("=== GridBASIC program listing ===");
    console_set_color(GRID_COL_DEFAULT);
    for (int i = 0; i < e->n; ++i) {
        console_write_line(e->lines[i]);
    }
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
    (void)e;
}

static void show_ai_panel(const char *title, const char *text) {
    console_clear();
    console_set_color(GRID_COL_TITLE);
    console_write_line(title);
    console_set_color(GRID_COL_DEFAULT);
    if (text && text[0]) {
        char line[200];
        size_t p = 0;
        for (size_t i = 0; ; ++i) {
            char c = text[i];
            if (c == '\n' || c == '\0') {
                line[p] = '\0';
                if (p > 0) {
                    console_write_line(line);
                }
                p = 0;
                if (c == '\0') {
                    break;
                }
            } else if (p + 1 < sizeof(line)) {
                line[p++] = c;
            }
        }
    }
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
}

static void cmd_ai(ide_t *e, const char *args) {
    char resp[640];
    if (args[0] == '\0' || sequal(args, "help") || sequal(args, "?")) {
        show_ai_panel("=== Grid AI (IDE) ===",
                      ":ai ask <prompt>     ask the AI\n"
                      ":ai explain          explain current line\n"
                      ":ai complete         complete buffer\n"
                      ":ai fix <code>       suggest fixed code\n"
                      ":ai models           list bridge model info\n"
                      ":ai <prompt>         shorthand for ask\n"
                      "Or: grid> ai ask ... (Flynn shell)");
        return;
    }
    if (starts_with(args, "ask ")) {
        ai_ask(args + 4, resp, sizeof(resp));
        show_ai_panel("=== AI ask ===", resp);
        return;
    }
    if (sequal(args, "ask")) {
        ide_status(e, ":ai ask <prompt>", GRID_COL_WARN);
        return;
    }
    if (sequal(args, "explain")) {
        ai_explain(e->lines[e->row], resp, sizeof(resp));
        show_ai_panel("=== AI explain ===", resp);
        return;
    }
    if (sequal(args, "complete")) {
        (void)serialize(e, g_ide_io_buf, sizeof(g_ide_io_buf));
        char src[4096];
        scopy(src, sizeof(src), g_ide_io_buf);
        ai_complete(src, resp, sizeof(resp));
        show_ai_panel("=== AI complete ===", resp);
        return;
    }
    if (sequal(args, "models") || sequal(args, "model")) {
        ai_models(resp, sizeof(resp));
        show_ai_panel("=== AI models ===", resp);
        return;
    }
    if (starts_with(args, "fix ")) {
        ai_fix(args + 4, resp, sizeof(resp));
        show_ai_panel("=== AI fix ===", resp);
        return;
    }
    /* bare prompt shorthand: :ai write a for loop */
    ai_ask(args, resp, sizeof(resp));
    show_ai_panel("=== AI ===", resp);
}

static uint16_t ide_parse_port(const char *text) {
    uint32_t port = 0;
    if (!text) {
        return 0;
    }
    for (size_t i = 0; text[i]; ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        port = port * 10u + (uint32_t)(text[i] - '0');
        if (port > 65535u) {
            return 0;
        }
    }
    return (uint16_t)port;
}

static int split_words(char *line, char *argv[], int max_args) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max_args) {
        while (*p == ' ') {
            p++;
        }
        if (!*p) {
            break;
        }
        argv[argc++] = p;
        while (*p && *p != ' ') {
            p++;
        }
        if (*p) {
            *p++ = '\0';
        }
    }
    return argc;
}

static int handle_vault_ide(ide_t *e, const char *line) {
    if (!security_require_capability(CAP_STORAGE, "vault")) {
        ide_status(e, "vault denied (need STORAGE cap)", GRID_COL_ERROR);
        return 1;
    }

    char buf[96];
    scopy(buf, sizeof(buf), line);
    if (starts_with(buf, "vault ")) {
        scopy(buf, sizeof(buf), line + 6);
    } else if (sequal(buf, "vault")) {
        ide_status(e, "vault list|get|put|sync", GRID_COL_WARN);
        return 1;
    }

    char *argv[6];
    int argc = split_words(buf, argv, 6);
    if (argc == 0) {
        ide_status(e, "vault list|get|put|sync", GRID_COL_WARN);
        return 1;
    }

    if (sequal(argv[0], "list")) {
        run_shell_line(e, "vault list");
        return 1;
    }
    if (sequal(argv[0], "get") && argc >= 2) {
        const char *value = storage_get(argv[1]);
        if (!value) {
            ide_status(e, "vault: node not found", GRID_COL_ERROR);
            return 1;
        }
        char msg[96];
        scopy(msg, sizeof(msg), argv[1]);
        scopy(msg + slen(msg), sizeof(msg) - slen(msg), "=");
        scopy(msg + slen(msg), sizeof(msg) - slen(msg), value);
        ide_status(e, msg, GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "put") && argc >= 3) {
        if (storage_put(argv[1], argv[2]) != 0) {
            ide_status(e, "vault put failed", GRID_COL_ERROR);
            return 1;
        }
        ide_status(e, "vault node stored", GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "sync")) {
        if (storage_sync_disk() != 0) {
            ide_status(e, "vault sync failed", GRID_COL_ERROR);
            return 1;
        }
        ide_status(e, "vault synced to disk", GRID_COL_OK);
        return 1;
    }

    ide_status(e, "vault list|get|put|sync", GRID_COL_WARN);
    return 1;
}

static int handle_irc_ide(ide_t *e, const char *line) {
    char buf[96];
    scopy(buf, sizeof(buf), line);
    if (starts_with(buf, "irc ")) {
        scopy(buf, sizeof(buf), line + 4);
    } else if (sequal(buf, "irc")) {
        ide_status(e, "irc connect|join|say|read|status|quit", GRID_COL_WARN);
        return 1;
    }
    char *argv[6];
    int argc = split_words(buf, argv, 6);
    if (argc == 0) {
        ide_status(e, "irc connect|join|say|read|status|quit", GRID_COL_WARN);
        return 1;
    }
    if (sequal(argv[0], "connect")) {
        if (argc < 4) {
            ide_status(e, "usage: irc connect ip port nick", GRID_COL_ERROR);
            return 1;
        }
        uint16_t port = ide_parse_port(argv[2]);
        if (port == 0 || irc_connect(argv[1], port, argv[3]) != 0) {
            ide_status(e, "IRC connect failed", GRID_COL_ERROR);
            return 1;
        }
        ide_status(e, "IRC connected", GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "join")) {
        if (argc < 2 || irc_join(argv[1]) != 0) {
            ide_status(e, "IRC join failed", GRID_COL_ERROR);
            return 1;
        }
        ide_status(e, "IRC joined channel", GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "part")) {
        if (argc < 2 || irc_part(argv[1]) != 0) {
            ide_status(e, "IRC part failed", GRID_COL_ERROR);
            return 1;
        }
        ide_status(e, "IRC parted channel", GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "say")) {
        if (argc < 3) {
            ide_status(e, "usage: irc say #chan msg", GRID_COL_ERROR);
            return 1;
        }
        char msg[160];
        size_t n = 0;
        for (int i = 2; i < argc && n + 1 < sizeof(msg); ++i) {
            if (i > 2 && n + 1 < sizeof(msg)) {
                msg[n++] = ' ';
            }
            const char *p = argv[i];
            while (*p && n + 1 < sizeof(msg)) {
                msg[n++] = *p++;
            }
        }
        msg[n] = '\0';
        if (irc_say(argv[1], msg) != 0) {
            ide_status(e, "IRC say failed", GRID_COL_ERROR);
            return 1;
        }
        ide_status(e, "IRC message sent", GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "read")) {
        irc_poll();
        char linebuf[IRC_LINE_MAX];
        if (irc_read(linebuf, sizeof(linebuf)) == 0) {
            ide_status(e, "no IRC messages", GRID_COL_DIM);
            return 1;
        }
        ide_status(e, linebuf, GRID_COL_DEFAULT);
        return 1;
    }
    if (sequal(argv[0], "status")) {
        irc_poll();
        char st[IRC_LINE_MAX];
        irc_status(st, sizeof(st));
        ide_status(e, st, GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "nick")) {
        if (argc < 2 || irc_nick(argv[1]) != 0) {
            ide_status(e, "IRC nick failed", GRID_COL_ERROR);
            return 1;
        }
        ide_status(e, "IRC nick changed", GRID_COL_OK);
        return 1;
    }
    if (sequal(argv[0], "quit") || sequal(argv[0], "disconnect")) {
        if (sequal(argv[0], "quit")) {
            irc_quit("GridBASIC IDE");
        } else {
            irc_disconnect();
        }
        ide_status(e, "IRC disconnected", GRID_COL_DIM);
        return 1;
    }
    ide_status(e, "unknown irc command", GRID_COL_ERROR);
    return 1;
}

static void cmd_help(void) {
    console_clear();
    console_set_color(GRID_COL_TITLE);
    console_write_line("=== GridBASIC IDE help ===");
    console_set_color(GRID_COL_DEFAULT);
    console_write_line("Editing:");
    console_write_line("  Arrows / Home / End   move cursor");
    console_write_line("  Enter                 split line (new line below)");
    console_write_line("  Backspace             delete char / merge line up");
    console_write_line("  Delete                delete char / merge line down");
    console_write_line("  Esc                   open grid> command line");
    console_write_line("Commands (Esc, then type at grid>):");
    console_write_line("  :run [path]           run buffer, .grid path, or file");
    console_write_line("  :save <name>          write to /programs/<name>.bas");
    console_write_line("  :load <name>          read /programs/<name>.bas or .grid");
    console_write_line("  :new                  clear the buffer");
    console_write_line("  :list                 print the program");
    console_write_line("  :mods [category]      list IDE modules (optional filter)");
    console_write_line("  :mod run <name>       run installed IDE module");
    console_write_line("  :mod load <name>      load module into editor");
    console_write_line("  :pkg list|mods|run|load  package manager from IDE");
    console_write_line("  :find <text>          search buffer from cursor");
    console_write_line("  :goto <line>          jump to line number");
    console_write_line("  :samples              list /programs/*.bas samples");
    console_write_line("  :redteam              red team lab (100 security demos)");
    console_write_line("  :blackhat             black hat lab (100 offensive demos)");
    console_write_line("  :tutorial             interactive GridBASIC walkthrough");
    console_write_line("  :compile <name>       compile buffer to /programs/<name>.grid");
    console_write_line("  Programs are limited to 65535 bytes for :run/:save/:compile");
    console_write_line("  :help                 this IDE help");
    console_write_line("  :ai ask <prompt>      AI help (host bridge or offline)");
    console_write_line("  :ai explain           explain current line");
    console_write_line("  :ai complete          suggest completion for buffer");
    console_write_line("  help                  Flynn Grid shell commands");
    console_write_line("  poweroff              exit Grid OS");
    console_write_line("GridBASIC statements:");
    console_write_line("  PRINT expr;expr   CONST v=expr   DIM A(10,10)   DATA/READ/RESTORE");
    console_write_line("  DEF FN f(x)=expr   SUB/END SUB   FUNCTION/END FUNCTION   CALL f(a)");
    console_write_line("  ELSEIF c THEN s   ON ERROR GOTO n   ON n GOTO a,b   OPTION BASE 0|1");
    console_write_line("  SELECT CASE x : CASE n .. : CASE ELSE .. : END SELECT");
    console_write_line("  IF c THEN s ELSE s  FOR i=a TO b .. NEXT   EXIT/CONTINUE FOR/WHILE");
    console_write_line("  WHILE c .. WEND   REPEAT .. UNTIL c   LINE INPUT s$");
    console_write_line("  MIN MAX FIX ROUND TRIM$ SPACE$ STRING$   LOCAL x   2D arrays");
    console_write_line("  RANDOMIZE [seed]   INSTR$(hay$,needle$)   GOTO n  GOSUB n");
    console_write_line("Grid bindings:");
    console_write_line("  GRID.DNS.RESOLVE$   GRID.NET.STATUS$   GRID.LOG.TAIL$(n)");
    console_write_line("  GRID.WHOAMI$   GRID.CAPS$   GRID.JOBS.LIST$/KILL   GRID.ISO.LIST$/SPAWN");
    console_write_line("  GRID.VAULT.* (+ EXPORT/IMPORT)   GRID.GFS.*   GRID.HTTP.*");
    console_write_line("  GRID.SPAWN / GRID.SPAWN.BG   GRID.LOCATE   GRID.INKEY$");
    console_write_line("  GRID.CLS  GRID.COLOR n  GRID.LOG msg  GRID.WAIT ticks");
    console_write_line("  GRID.SPAWN \"name\"  GRID.TIME  GRID.PING(ip$)  GRID.CAP(n)");
    console_write_line("  GRID.AI.ASK$/PRINT   GRID.IRC.*   GRID.BTC.*");
    console_write_line("  GRID.PKG.LIST$/MODS$   GRID.PKG.INSTALL/REMOVE/MOD.RUN/RECV");
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
}

static int handle_ide_command(ide_t *e, const char *cmd) {
    if (sequal(cmd, "run") || sequal(cmd, "r")) { run_buffer(e, 0); return 1; }
    if (starts_with(cmd, "run ")) { run_buffer(e, cmd + 4); return 1; }
    if (sequal(cmd, "quit") || sequal(cmd, "q")) {
        ide_status(e, "use grid> poweroff to exit", GRID_COL_WARN);
        return 1;
    }
    if (sequal(cmd, "new")) {
        e->n = 1; e->lines[0][0] = '\0'; e->row = 0; e->col = 0; e->top = 0;
        e->path[0] = '\0'; e->dirty = 0;
        ide_status(e, "new buffer", GRID_COL_OK); return 1;
    }
    if (sequal(cmd, "list") || sequal(cmd, "l")) { cmd_list(e); return 1; }
    if (sequal(cmd, "mods")) { cmd_mods_list(e, 0); return 1; }
    if (starts_with(cmd, "mods ")) { cmd_mods_list(e, cmd + 5); return 1; }
    if (sequal(cmd, "pkg") || starts_with(cmd, "pkg ")) {
        cmd_pkg(e, sequal(cmd, "pkg") ? "" : cmd + 4);
        return 1;
    }
    if (starts_with(cmd, "find ")) { cmd_find(e, cmd + 5); return 1; }
    if (starts_with(cmd, "goto ")) { cmd_goto(e, cmd + 5); return 1; }
    if (sequal(cmd, "find")) { ide_status(e, "usage: find <text>", GRID_COL_ERROR); return 1; }
    if (sequal(cmd, "goto")) { ide_status(e, "usage: goto <line>", GRID_COL_ERROR); return 1; }
    if (starts_with(cmd, "mod run ")) { cmd_mod_run(e, cmd + 8); return 1; }
    if (starts_with(cmd, "mod load ")) { cmd_mod_load(e, cmd + 9); return 1; }
    if (sequal(cmd, "samples")) { run_shell_line(e, "samples"); return 1; }
    if (sequal(cmd, "redteam")) { run_shell_line(e, "redteam"); return 1; }
    if (sequal(cmd, "blackhat")) { run_shell_line(e, "blackhat"); return 1; }
    if (sequal(cmd, "tutorial") || sequal(cmd, "t")) { cmd_tutorial_steps(e); return 1; }
    if (starts_with(cmd, "compile ")) { cmd_compile(e, cmd + 8); return 1; }
    if (starts_with(cmd, "compile")) { cmd_compile(e, cmd + 7); return 1; }
    if (sequal(cmd, "help") || sequal(cmd, "h") || sequal(cmd, "?")) { cmd_help(); return 1; }
    if (starts_with(cmd, "save ")) { cmd_save(e, cmd + 5); return 1; }
    if (starts_with(cmd, "load ")) { cmd_load(e, cmd + 5); return 1; }
    if (starts_with(cmd, "save")) { cmd_save(e, cmd + 4); return 1; }
    if (starts_with(cmd, "load")) { cmd_load(e, cmd + 4); return 1; }
    if (sequal(cmd, "ai")) { cmd_ai(e, ""); return 1; }
    if (starts_with(cmd, "ai ")) { cmd_ai(e, cmd + 3); return 1; }
    if (sequal(cmd, "btc")) { run_shell_line(e, "btc"); return 1; }
    if (starts_with(cmd, "btc ")) { run_shell_line(e, cmd); return 1; }
    if (sequal(cmd, "vault") || starts_with(cmd, "vault ")) { return handle_vault_ide(e, cmd); }
    if (sequal(cmd, "irc") || starts_with(cmd, "irc ")) { return handle_irc_ide(e, cmd); }
    return 0;
}

static void run_shell_line(ide_t *e, const char *line) {
    char buf[100];
    scopy(buf, sizeof(buf), line);
    console_clear();
    shell_dispatch_line(buf);
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key to return to GridBASIC ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
    ide_redraw(e);
}

static void handle_command(ide_t *e) {
    char cmd[100];
    e->status[0] = '\0';   /* opening the prompt clears the old status */
    read_cmd(cmd, sizeof(cmd), IDE_SHELL_PROMPT);
    if (cmd[0] == '\0') {
        return;
    }

    if (cmd[0] == ':') {
        if (handle_ide_command(e, cmd + 1)) {
            return;
        }
        ide_status(e, "unknown :command (try :help)", GRID_COL_ERROR);
        return;
    }

    if (sequal(cmd, "irc") || starts_with(cmd, "irc ")) {
        handle_irc_ide(e, cmd);
        return;
    }
    if (sequal(cmd, "btc") || starts_with(cmd, "btc ")) {
        run_shell_line(e, cmd);
        return;
    }
    if (sequal(cmd, "vault") || starts_with(cmd, "vault ")) {
        handle_vault_ide(e, cmd);
        return;
    }

    run_shell_line(e, cmd);
}

int basic_ide(const char *path) {
    shell_set_in_basic_ide(1);
    ide.n = 1; ide.lines[0][0] = '\0';
    ide.row = 0; ide.col = 0; ide.top = 0; ide.dirty = 0;
    ide.path[0] = '\0';
    ide.status[0] = '\0'; ide.status_attr = GRID_COL_DIM;
    if (g_boot_hint[0]) {
        scopy(ide.status, sizeof(ide.status), g_boot_hint);
        ide.status_attr = GRID_COL_OK;
        g_boot_hint[0] = '\0';
    }
    if (path && path[0]) {
        int lrc = load_path_into(&ide, path);
        if (lrc == 2) {
            scopy(ide.status, sizeof(ide.status), "load truncated at 65536 bytes");
            ide.status_attr = GRID_COL_WARN;
        } else if (lrc < 0) {
            scopy(ide.path, sizeof(ide.path), path);
        }
    }

    int running = 1;
    ide_redraw(&ide);
    while (running) {
        int k = console_read_key();
        if (k == 27) {
            handle_command(&ide);
            ide_redraw(&ide);
            continue;
        }
        if (k == (int)CONSOLE_CTRL_C) { continue; }
        if (k >= 0x100) {
            switch (k) {
            case CONSOLE_SC_UP:    ide.row--; clamp_cursor(&ide); break;
            case CONSOLE_SC_DOWN:  ide.row++; clamp_cursor(&ide); break;
            case CONSOLE_SC_LEFT:  ide.col--; clamp_cursor(&ide); break;
            case CONSOLE_SC_RIGHT: ide.col++; clamp_cursor(&ide); break;
            case CONSOLE_SC_HOME:  ide.col = 0; break;
            case CONSOLE_SC_END:   ide.col = (int)slen(ide.lines[ide.row]); break;
            case CONSOLE_SC_DEL:   delete_char(&ide, ide.row, ide.col); ide.dirty = 1; break;
            default: break;
            }
            ensure_visible(&ide);
            ide_redraw(&ide);
            continue;
        }
        char c = (char)k;
        if (c == '\n') {
            split_line(&ide, ide.row, ide.col);
            ide.row++; ide.col = 0; ide.dirty = 1;
            clamp_cursor(&ide); ensure_visible(&ide);
            ide_redraw(&ide);
            continue;
        }
        if (c == '\b') {
            if (ide.col == 0) {
                merge_up(&ide, ide.row);
            } else {
                ide.col--;
                delete_char(&ide, ide.row, ide.col);
            }
            ide.dirty = 1;
            clamp_cursor(&ide); ensure_visible(&ide);
            ide_redraw(&ide);
            continue;
        }
        if (c == '\t') {
            for (int s = 0; s < 4; ++s) insert_char_at(&ide, ide.row, ide.col, ' ');
            ide.col += 4; ide.dirty = 1;
            clamp_cursor(&ide); ensure_visible(&ide);
            ide_redraw(&ide);
            continue;
        }
        if (c >= 32 && c < 127) {
            insert_char_at(&ide, ide.row, ide.col, c);
            ide.col++; ide.dirty = 1;
            clamp_cursor(&ide); ensure_visible(&ide);
            ide_redraw(&ide);
            continue;
        }
    }
    shell_set_in_basic_ide(0);
    console_clear();
    return 0;
}
