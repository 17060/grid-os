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

static int pp_line_active = 1;
static int pp_if_stack[16];
static int pp_if_sp = 0;

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

static int pp_emit_line(const char *line, char *out, size_t *pos, size_t cap) {
    size_t i = 0;
    if (!pp_line_active) {
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

static int pp_handle_directive(char *line, char *out, size_t *pos, size_t cap, int depth) {
    char *p = line;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (p[0] != '#' || p[1] != 'I') {
        return -1;
    }

    if (p[2] == 'F' && (p[3] == ' ' || p[3] == '\t')) {
        p += 3;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        int val = pp_eval_if(p);
        if (pp_if_sp < 16) {
            pp_if_stack[pp_if_sp++] = pp_line_active;
        }
        if (!pp_line_active) {
            return 0;
        }
        pp_line_active = val ? 1 : 0;
        return 0;
    }

    if (p[2] == 'E' && p[3] == 'L' && p[4] == 'S' && p[5] == 'E') {
        if (pp_if_sp <= 0) {
            return 0;
        }
        int parent = pp_if_stack[pp_if_sp - 1];
        pp_line_active = parent && !pp_line_active ? 1 : 0;
        return 0;
    }

    if (p[2] == 'E' && p[3] == 'N' && p[4] == 'D' && p[5] == 'I' && p[6] == 'F') {
        if (pp_if_sp > 0) {
            pp_line_active = pp_if_stack[--pp_if_sp];
        }
        return 0;
    }

    if (p[2] == 'N' && p[3] == 'C' && p[4] == 'L' && p[5] == 'U' && p[6] == 'D' && p[7] == 'E') {
        p += 8;
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
        if (!pp_line_active || depth >= 4) {
            return 0;
        }
        static char incbuf[4096];
        static char incout[4096];
        size_t got = 0;
        if (gfs_read_file(path, incbuf, sizeof(incbuf) - 1, &got) != 0 || got == 0) {
            return 0;
        }
        incbuf[got] = '\0';
        if (basic_preprocess(incbuf, incout, sizeof(incout)) != 0) {
            return 0;
        }
        for (size_t j = 0; incout[j] && *pos + 1 < cap; ++j) {
            out[(*pos)++] = incout[j];
        }
        return 0;
    }

    return -1;
}

int basic_preprocess(const char *src, char *out, size_t out_cap) {
    size_t pos = 0;
    char line[256];
    size_t li = 0;

    if (!src || !out || out_cap < 2) {
        return -1;
    }

    pp_if_sp = 0;
    pp_line_active = 1;

    for (size_t i = 0; src[i] || li > 0; ++i) {
        char c = src[i];
        if (c == '\0' && li == 0) {
            break;
        }
        if (c == '\n' || c == '\r' || c == '\0') {
            line[li] = '\0';
            pp_trim(line);
            if (line[0] == '#') {
                (void)pp_handle_directive(line, out, &pos, out_cap, 0);
            } else {
                (void)pp_emit_line(line, out, &pos, out_cap);
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
