#include "basic_pp.h"

#include "gfs.h"

#include <stddef.h>

static int pp_streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static void pp_trim(char *s) {
    size_t n = 0;
    while (s[n]) {
        n++;
    }
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
    size_t i = 0;
    while (s[i] == ' ' || s[i] == '\t') {
        i++;
    }
    if (i > 0) {
        size_t j = 0;
        while (s[i]) {
            s[j++] = s[i++];
        }
        s[j] = '\0';
    }
}

/* Per-invocation preprocessor state: each (possibly nested) file gets its
 * own #IF nesting so an included file cannot clobber its parent's state. */
typedef struct {
    int line_active;
    int if_stack[16];
    int if_sp;
} pp_state_t;

/* #INCLUDE nesting limit; also sizes the per-depth include buffers. */
#define PP_INCLUDE_DEPTH_MAX 4

/* One buffer pair per nesting level (static: far too large for the kernel
 * stack). Level d includes a file into inc_buf[d]/inc_out[d]; a nested
 * include inside it runs at depth d+1 and uses the next pair. */
static char inc_buf[PP_INCLUDE_DEPTH_MAX][4096];
static char inc_out[PP_INCLUDE_DEPTH_MAX][4096];

static int pp_run(const char *src, char *out, size_t out_cap, int depth);

static int pp_eval_if(const char *expr) {
    if (!expr || !expr[0]) {
        return 0;
    }
    if (pp_streq(expr, "1") || pp_streq(expr, "TRUE") || pp_streq(expr, "true")) {
        return 1;
    }
    if (pp_streq(expr, "GRIDOS") || pp_streq(expr, "GRIDOS7")) {
        return 1;
    }
    if (expr[0] >= '0' && expr[0] <= '9') {
        return expr[0] != '0';
    }
    return 0;
}

static int pp_emit_line(pp_state_t *st, const char *line, char *out, size_t *pos, size_t cap) {
    size_t i = 0;
    if (!st->line_active) {
        return 0;
    }
    while (line[i] && *pos + 1 < cap) {
        out[(*pos)++] = line[i++];
    }
    if (*pos + 1 < cap) {
        out[(*pos)++] = '\n';
    }
    return 0;
}

/* Match `word` at *pp (directive keyword). On success advance *pp past it
 * and return 1. `need_ws` requires whitespace after the keyword. */
static int pp_match_word(char **pp, const char *word, int need_ws) {
    char *p = *pp;
    while (*word) {
        if (*p != *word) {
            return 0;
        }
        p++;
        word++;
    }
    if (need_ws && *p != ' ' && *p != '\t') {
        return 0;
    }
    *pp = p;
    return 1;
}

static int pp_handle_directive(pp_state_t *st, char *line, char *out, size_t *pos,
                               size_t cap, int depth) {
    char *p = line;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (p[0] != '#') {
        return -1;
    }
    p++;

    if (pp_match_word(&p, "IF", 1)) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        int val = pp_eval_if(p);
        if (st->if_sp < 16) {
            st->if_stack[st->if_sp++] = st->line_active;
        }
        if (!st->line_active) {
            return 0;
        }
        st->line_active = val ? 1 : 0;
        return 0;
    }

    if (pp_match_word(&p, "ELSE", 0)) {
        if (st->if_sp <= 0) {
            return 0;
        }
        int parent = st->if_stack[st->if_sp - 1];
        st->line_active = parent && !st->line_active ? 1 : 0;
        return 0;
    }

    if (pp_match_word(&p, "ENDIF", 0)) {
        if (st->if_sp > 0) {
            st->line_active = st->if_stack[--st->if_sp];
        }
        return 0;
    }

    if (pp_match_word(&p, "INCLUDE", 0)) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p != '"') {
            return 0;
        }
        p++;
        char path[64];
        size_t k = 0;
        while (*p && *p != '"' && k + 1 < sizeof(path)) {
            path[k++] = *p++;
        }
        path[k] = '\0';
        if (!st->line_active || depth >= PP_INCLUDE_DEPTH_MAX) {
            return 0;
        }
        char *ibuf = inc_buf[depth];
        char *iout = inc_out[depth];
        size_t got = 0;
        if (gfs_read_file(path, ibuf, sizeof(inc_buf[0]) - 1, &got) != 0 || got == 0) {
            return 0;
        }
        ibuf[got] = '\0';
        if (pp_run(ibuf, iout, sizeof(inc_out[0]), depth + 1) != 0) {
            return 0;
        }
        for (size_t j = 0; iout[j] && *pos + 1 < cap; ++j) {
            out[(*pos)++] = iout[j];
        }
        return 0;
    }

    return -1;
}

static int pp_run(const char *src, char *out, size_t out_cap, int depth) {
    size_t pos = 0;
    char line[256];
    size_t li = 0;
    pp_state_t st;

    if (!src || !out || out_cap < 2) {
        return -1;
    }

    st.line_active = 1;
    st.if_sp = 0;

    for (size_t i = 0; src[i] || li > 0; ++i) {
        char c = src[i];
        if (c == '\0' && li == 0) {
            break;
        }
        if (c == '\n' || c == '\r' || c == '\0') {
            line[li] = '\0';
            pp_trim(line);
            if (line[0] == '#') {
                (void)pp_handle_directive(&st, line, out, &pos, out_cap, depth);
            } else {
                (void)pp_emit_line(&st, line, out, &pos, out_cap);
            }
            li = 0;
            if (c == '\0') {
                break;
            }
            continue;
        }
        if (li + 1 < sizeof(line)) {
            line[li++] = c;
        }
    }

    out[pos] = '\0';
    return 0;
}

int basic_preprocess(const char *src, char *out, size_t out_cap) {
    return pp_run(src, out, out_cap, 0);
}
