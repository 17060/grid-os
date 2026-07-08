/* Host test for kernel/basic_pp.c #INCLUDE recursion + state isolation. */
#include <stdio.h>
#include <string.h>
#include "basic_pp.h"

/* gfs stub: serve fake files by path */
static const char *self_inc =
    "10 PRINT \"self\"\n"
    "#INCLUDE \"/programs/self.bas\"\n";

static const char *child_with_if =
    "#IF 0\n"
    "20 PRINT \"hidden\"\n"
    "#ENDIF\n"
    "30 PRINT \"child\"\n";

int gfs_read_file(const char *path, void *out, size_t cap, size_t *len) {
    const char *body = 0;
    if (strcmp(path, "/programs/self.bas") == 0) body = self_inc;
    if (strcmp(path, "/programs/child.bas") == 0) body = child_with_if;
    if (!body) return -1;
    size_t n = strlen(body);
    if (n > cap) n = cap;
    memcpy(out, body, n);
    if (len) *len = n;
    return 0;
}

static int count_occurrences(const char *hay, const char *needle) {
    int c = 0;
    for (const char *p = hay; (p = strstr(p, needle)) != 0; p += strlen(needle)) c++;
    return c;
}

int main(void) {
    static char out[65536];

    /* 1. Self-including file must terminate, bounded by the depth guard:
       top level + 4 include levels = 5 emissions of the marker line. */
    if (basic_preprocess(self_inc, out, sizeof(out)) != 0) {
        printf("FAIL: self-include preprocess errored\n");
        return 1;
    }
    int n = count_occurrences(out, "PRINT \"self\"");
    if (n != 5) {
        printf("FAIL: expected 5 self markers (top + 4 depths), got %d\n", n);
        return 1;
    }

    /* 2. A child's #IF state must not clobber the parent: parent is inside
       an active #IF, includes a child that ends in an inactive #IF block
       closed by #ENDIF; parent lines after the include must still emit,
       and the parent's own #ELSE must still be suppressed. */
    const char *parent =
        "#IF 1\n"
        "10 PRINT \"before\"\n"
        "#INCLUDE \"/programs/child.bas\"\n"
        "40 PRINT \"after\"\n"
        "#ELSE\n"
        "50 PRINT \"wrong-branch\"\n"
        "#ENDIF\n";
    if (basic_preprocess(parent, out, sizeof(out)) != 0) {
        printf("FAIL: nested preprocess errored\n");
        return 1;
    }
    if (!strstr(out, "PRINT \"before\"") || !strstr(out, "PRINT \"child\"") ||
        !strstr(out, "PRINT \"after\"")) {
        printf("FAIL: expected parent+child lines missing:\n%s\n", out);
        return 1;
    }
    if (strstr(out, "hidden") || strstr(out, "wrong-branch")) {
        printf("FAIL: inactive branch leaked:\n%s\n", out);
        return 1;
    }

    printf("pp tests OK\n");
    return 0;
}
