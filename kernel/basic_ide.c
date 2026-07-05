#include "basic.h"

#include "console.h"
#include "gfs.h"
#include "shell.h"

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
 *   grid> :help         IDE editing help
 * ========================================================================== */

#define IDE_MAX_LINES  400
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

static ide_t ide;

static size_t slen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static void scopy(char *d, size_t cap, const char *s) {
    size_t n = 0; while (s[n] && n + 1 < cap) { d[n] = s[n]; n++; } d[n] = '\0';
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
        console_write_at(6, y, e->lines[ln], GRID_COL_DEFAULT);
    }
}

static void draw_footer(ide_t *e) {
    console_fill_row(IDE_FOOTER_ROW, ' ', GRID_COL_MENU);
    const char *bar = " ESC: grid> command   Arrows: move   Enter: new line   Del/Bksp: edit ";
    console_write_at(0, IDE_FOOTER_ROW, bar, GRID_COL_MENU);
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

static void read_cmd(char *out, size_t cap, const char *prompt) {
    size_t prompt_len = slen(prompt);
    console_fill_row(IDE_CMD_ROW, ' ', GRID_COL_DEFAULT);
    console_write_at(0, IDE_CMD_ROW, prompt, GRID_COL_OK);
    size_t len = 0;
    int col = (int)prompt_len;
    for (;;) {
        int k = console_read_key();
        if (k >= 0x100) {
            if (k == CONSOLE_SC_LEFT && col > (int)prompt_len) { col--; continue; }
            if (k == CONSOLE_SC_RIGHT && (size_t)col < prompt_len + len) { col++; continue; }
            if (k == CONSOLE_SC_HOME) { col = (int)prompt_len; continue; }
            if (k == CONSOLE_SC_END) { col = (int)prompt_len + (int)len; continue; }
            if (k == CONSOLE_SC_DEL) {
                if ((size_t)col < prompt_len + len) {
                    size_t idx = (size_t)col - prompt_len;
                    for (size_t i = idx; i < len; ++i) out[i] = out[i + 1];
                    len--; out[len] = '\0';
                    console_write_at(prompt_len, IDE_CMD_ROW, out, GRID_COL_DEFAULT);
                    console_fill_row(IDE_CMD_ROW, ' ', GRID_COL_DEFAULT);
                    console_write_at(0, IDE_CMD_ROW, prompt, GRID_COL_OK);
                    console_write_at(prompt_len, IDE_CMD_ROW, out, GRID_COL_DEFAULT);
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
                console_fill_row(IDE_CMD_ROW, ' ', GRID_COL_DEFAULT);
                console_write_at(0, IDE_CMD_ROW, prompt, GRID_COL_OK);
                console_write_at(prompt_len, IDE_CMD_ROW, out, GRID_COL_DEFAULT);
            }
            continue;
        }
        if (c >= 32 && c < 127 && len + 1 < cap) {
            size_t idx = (size_t)col - prompt_len;
            for (size_t i = len + 1; i > idx; --i) out[i] = out[i - 1];
            out[idx] = c; len++; col++;
            console_fill_row(IDE_CMD_ROW, ' ', GRID_COL_DEFAULT);
            console_write_at(0, IDE_CMD_ROW, prompt, GRID_COL_OK);
            console_write_at(prompt_len, IDE_CMD_ROW, out, GRID_COL_DEFAULT);
        }
    }
}

/* serialize buffer into a single source string */
static void serialize(ide_t *e, char *out, size_t cap) {
    size_t p = 0;
    for (int i = 0; i < e->n && p + 2 < cap; ++i) {
        const char *l = e->lines[i];
        size_t len = slen(l);
        for (size_t j = 0; j < len && p + 2 < cap; ++j) out[p++] = l[j];
        if (p + 1 < cap) out[p++] = '\n';
    }
    out[p] = '\0';
}

static void run_buffer(ide_t *e) {
    static char src[8192];
    serialize(e, src, sizeof(src));
    console_clear();
    console_set_color(GRID_COL_TITLE);
    console_write_line("=== GridBASIC run ===");
    console_set_color(GRID_COL_DEFAULT);
    basic_run_source(src);
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key to return to IDE ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
    (void)e;
}

static void make_path(const char *name, char *out, size_t cap) {
    if (starts_with(name, "/")) { scopy(out, cap, name); return; }
    size_t p = 0;
    const char *pre = "/programs/";
    while (*pre && p + 1 < cap) out[p++] = *pre++;
    const char *s = name;
    while (*s && p + 1 < cap) out[p++] = *s++;
    /* ensure .bas suffix */
    if (!(p >= 4 && out[p-4]=='.' && out[p-3]=='b' && out[p-2]=='a' && out[p-1]=='s')) {
        const char *suf = ".bas";
        while (*suf && p + 1 < cap) out[p++] = *suf++;
    }
    out[p] = '\0';
}

static void cmd_save(ide_t *e, const char *name) {
    if (!name[0]) { ide_status(e, "usage: save <name>", GRID_COL_ERROR); return; }
    char path[80]; make_path(name, path, sizeof(path));
    static char src[8192]; serialize(e, src, sizeof(src));
    if (gfs_write_file(path, src, slen(src)) != 0) {
        ide_status(e, "save failed (GFS write denied)", GRID_COL_ERROR); return;
    }
    scopy(e->path, sizeof(e->path), path);
    e->dirty = 0;
    char msg[80]; scopy(msg, sizeof(msg), "saved "); scopy(msg + 6, sizeof(msg) - 6, path);
    ide_status(e, msg, GRID_COL_OK);
}

static void cmd_load(ide_t *e, const char *name) {
    if (!name[0]) { ide_status(e, "usage: load <name>", GRID_COL_ERROR); return; }
    char path[80]; make_path(name, path, sizeof(path));
    static char buf[8192]; size_t got = 0;
    if (gfs_read_file(path, buf, sizeof(buf) - 1, &got) != 0) {
        ide_status(e, "load failed (not found)", GRID_COL_ERROR); return;
    }
    buf[got] = '\0';
    e->n = 0; e->row = 0; e->col = 0; e->top = 0;
    size_t i = 0;
    int row = 0;
    size_t col = 0;
    e->lines[0][0] = '\0';
    while (buf[i] && row < IDE_MAX_LINES) {
        char c = buf[i++];
        if (c == '\n') { row++; col = 0; if (row < IDE_MAX_LINES) e->lines[row][0] = '\0'; }
        else if (c == '\r') { /* skip */ }
        else if (col + 1 < IDE_LINE_LEN) { e->lines[row][col++] = c; e->lines[row][col] = '\0'; }
    }
    if (row >= IDE_MAX_LINES) row = IDE_MAX_LINES - 1;
    e->n = row + 1;
    if (e->n == 0) { e->n = 1; e->lines[0][0] = '\0'; }
    scopy(e->path, sizeof(e->path), path);
    e->dirty = 0;
    char msg[80]; scopy(msg, sizeof(msg), "loaded "); scopy(msg + 7, sizeof(msg) - 7, path);
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
    console_write_line("  :run                  run the program in the buffer");
    console_write_line("  :save <name>          write to /programs/<name>.bas");
    console_write_line("  :load <name>          read /programs/<name>.bas");
    console_write_line("  :new                  clear the buffer");
    console_write_line("  :list                 print the program");
    console_write_line("  :help                 this IDE help");
    console_write_line("  help                  Flynn Grid shell commands");
    console_write_line("  poweroff              exit Grid OS");
    console_write_line("GridBASIC statements:");
    console_write_line("  PRINT expr;expr   LET v=expr   INPUT v   DIM A(10)");
    console_write_line("  IF c THEN s ELSE s  FOR i=a TO b STEP s .. NEXT [i]");
    console_write_line("  WHILE c .. WEND   REPEAT .. UNTIL c   GOTO n  GOSUB n/RETURN");
    console_write_line("Grid bindings:");
    console_write_line("  GRID.CLS  GRID.COLOR n  GRID.LOG msg  GRID.WAIT ticks");
    console_write_line("  GRID.SPAWN \"name\"  GRID.SERIAL.WRITE s$");
    console_write_line("  GRID.TIME  GRID.RND(n)  GRID.PING(ip$)  GRID.STATUS$");
    console_set_color(GRID_COL_DIM);
    console_write_line("--- press any key ---");
    console_set_color(GRID_COL_DEFAULT);
    (void)console_read_key();
}

static int handle_ide_command(ide_t *e, const char *cmd) {
    if (sequal(cmd, "run") || sequal(cmd, "r")) { run_buffer(e); return 1; }
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
    if (sequal(cmd, "help") || sequal(cmd, "h") || sequal(cmd, "?")) { cmd_help(); return 1; }
    if (starts_with(cmd, "save ")) { cmd_save(e, cmd + 5); return 1; }
    if (starts_with(cmd, "load ")) { cmd_load(e, cmd + 5); return 1; }
    if (starts_with(cmd, "save")) { cmd_save(e, cmd + 4); return 1; }
    if (starts_with(cmd, "load")) { cmd_load(e, cmd + 4); return 1; }
    return 0;
}

static void run_shell_line(ide_t *e, char *line) {
    console_clear();
    shell_dispatch_line(line);
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

    run_shell_line(e, cmd);
}

int basic_ide(const char *path) {
    shell_set_in_basic_ide(1);
    ide.n = 1; ide.lines[0][0] = '\0';
    ide.row = 0; ide.col = 0; ide.top = 0; ide.dirty = 0;
    ide.path[0] = '\0';
    ide.status[0] = '\0'; ide.status_attr = GRID_COL_DIM;
    if (path && path[0]) {
        char buf[8192]; size_t got = 0;
        if (gfs_read_file(path, buf, sizeof(buf) - 1, &got) == 0) {
            buf[got] = '\0';
            size_t i = 0; int row = 0; size_t col = 0;
            while (buf[i] && row < IDE_MAX_LINES) {
                char c = buf[i++];
                if (c == '\n') { row++; col = 0; if (row < IDE_MAX_LINES) ide.lines[row][0] = '\0'; }
                else if (c == '\r') {}
                else if (col + 1 < IDE_LINE_LEN) { ide.lines[row][col++] = c; ide.lines[row][col] = '\0'; }
            }
            ide.n = row + 1;
            scopy(ide.path, sizeof(ide.path), path);
        } else {
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
