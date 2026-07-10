#include "disc.h"

#include "storage.h"

#include <stddef.h>

static int disc_level_val = 1;
static int disc_xp_val = 0;

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static int parse_int(const char *s, int def) {
    int v = 0;
    int ok = 0;
    if (!s) {
        return def;
    }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        ok = 1;
        s++;
    }
    return ok ? v : def;
}

void disc_init(void) {
    const char *lv = storage_get("disc_level");
    const char *xp = storage_get("disc_xp");
    const char *ent = storage_get("disc_entity");
    disc_level_val = parse_int(lv, 1);
    disc_xp_val = parse_int(xp, 0);
    if (disc_level_val < 1) {
        disc_level_val = 1;
    }
    if (!ent || ent[0] == '\0') {
        storage_put("disc_entity", "User");
    }
}

static void disc_save(void) {
    char b[16];
    size_t n = 0;
    int v = disc_level_val;
    if (v == 0) {
        b[n++] = '0';
    } else {
        char t[12];
        int k = 0;
        while (v > 0) {
            t[k++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (k > 0 && n + 1 < sizeof(b)) {
            b[n++] = t[--k];
        }
    }
    b[n] = '\0';
    storage_put("disc_level", b);
    v = disc_xp_val;
    n = 0;
    if (v == 0) {
        b[n++] = '0';
    } else {
        char t[12];
        int k = 0;
        while (v > 0) {
            t[k++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (k > 0 && n + 1 < sizeof(b)) {
            b[n++] = t[--k];
        }
    }
    b[n] = '\0';
    storage_put("disc_xp", b);
}

static void disc_add_xp(int n) {
    disc_xp_val += n;
    while (disc_xp_val >= disc_level_val * 100) {
        disc_xp_val -= disc_level_val * 100;
        disc_level_val++;
        if (disc_level_val >= 99) {
            disc_level_val = 99;
            break;
        }
        const char *ent = storage_get("disc_entity");
        if (disc_level_val >= 10 && ent && streq(ent, "User")) {
            storage_put("disc_entity", "Program");
        }
    }
    disc_save();
}

void disc_on_program_run(const char *name) {
    (void)name;
    disc_add_xp(5);
}

void disc_on_basic_run(void) {
    disc_add_xp(1);
}

void disc_on_module_run(const char *name) {
    (void)name;
    disc_add_xp(2);
}

void disc_on_duel(void) {
    disc_add_xp(10);
}

void disc_on_lab_complete(void) {
    /* OS-internals labs are the hardest achievement on the Grid — implementing
     * and understanding kernel internals — so they grant the most disc XP. */
    disc_add_xp(40);
}

int disc_level(void) {
    return disc_level_val;
}

int disc_xp(void) {
    return disc_xp_val;
}

const char *disc_entity(void) {
    const char *ent = storage_get("disc_entity");
    return ent ? ent : "User";
}

void disc_format_status(char *out, size_t cap) {
    if (!out || cap == 0) {
        return;
    }
    size_t n = 0;
    const char *parts[] = {
        "Disc L", 0, " XP ", 0, " ", disc_entity(), 0
    };
    char lv[8];
    char xp[8];
    size_t li = 0;
    int v = disc_level_val;
    if (v == 0) {
        lv[li++] = '0';
    } else {
        char t[8];
        int k = 0;
        while (v > 0) {
            t[k++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (k > 0) {
            lv[li++] = t[--k];
        }
    }
    lv[li] = '\0';
    li = 0;
    v = disc_xp_val;
    if (v == 0) {
        xp[li++] = '0';
    } else {
        char t[8];
        int k = 0;
        while (v > 0) {
            t[k++] = (char)('0' + (v % 10));
            v /= 10;
        }
        while (k > 0) {
            xp[li++] = t[--k];
        }
    }
    xp[li] = '\0';
    parts[1] = lv;
    parts[3] = xp;
    for (int i = 0; i < 6; ++i) {
        const char *s = parts[i];
        if (!s) {
            continue;
        }
        while (*s && n + 1 < cap) {
            out[n++] = *s++;
        }
    }
    out[n] = '\0';
}
