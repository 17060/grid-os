#include "pkg.h"

#include "basic.h"
#include "console.h"
#include "gfs.h"
#include "log.h"
#include "security.h"
#include "serial.h"
#include "storage.h"

#include <stddef.h>
#include <stdint.h>

#define PKG_MAX       8
#define PKG_MOD_MAX   32
#define PKG_FILE_MAX  48
#define PKG_NAME_MAX  24
#define PKG_PATH_MAX  GFS_PATH_MAX
#define GRIDLINK_HDR  "GRIDLINK/1.0"
#define GRIDLINK_PKG  GRIDLINK_HDR "/PKG"
#define GRIDLINK_END  "#GRIDLINK/END"

typedef struct {
    int used;
    char name[PKG_NAME_MAX];
    char version[16];
    char desc[64];
    char manifest[PKG_PATH_MAX];
    char files[PKG_FILE_MAX][PKG_PATH_MAX];
    int nfiles;
} pkg_entry_t;

typedef struct {
    int used;
    char name[PKG_NAME_MAX];
    char path[PKG_PATH_MAX];
    char desc[48];
    char pkg[PKG_NAME_MAX];
} pkg_mod_t;

static pkg_entry_t g_pkgs[PKG_MAX];
static pkg_mod_t g_mods[PKG_MOD_MAX];

static int pkg_streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static int pkg_prefix(const char *line, const char *pfx) {
    while (*pfx) {
        if (*line++ != *pfx++) {
            return 0;
        }
    }
    return 1;
}

static void pkg_trim(char *s) {
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

static void pkg_copy(char *d, size_t cap, const char *s) {
    size_t n = 0;
    while (s[n] && n + 1 < cap) {
        d[n] = s[n];
        n++;
    }
    d[n] = '\0';
}

static int pkg_ends_manifest(const char *path) {
    size_t n = 0;
    while (path[n]) {
        n++;
    }
    if (n < 9) {
        return 0;
    }
    return pkg_streq(path + n - 9, "/MANIFEST");
}

static pkg_entry_t *pkg_find(const char *name) {
    for (int i = 0; i < PKG_MAX; ++i) {
        if (g_pkgs[i].used && pkg_streq(g_pkgs[i].name, name)) {
            return &g_pkgs[i];
        }
    }
    return 0;
}

static pkg_entry_t *pkg_alloc(void) {
    for (int i = 0; i < PKG_MAX; ++i) {
        if (!g_pkgs[i].used) {
            g_pkgs[i].used = 1;
            g_pkgs[i].nfiles = 0;
            g_pkgs[i].name[0] = '\0';
            g_pkgs[i].version[0] = '\0';
            g_pkgs[i].desc[0] = '\0';
            g_pkgs[i].manifest[0] = '\0';
            return &g_pkgs[i];
        }
    }
    return 0;
}

static void pkg_clear_mods_for(const char *pkg_name) {
    for (int i = 0; i < PKG_MOD_MAX; ++i) {
        if (g_mods[i].used && pkg_streq(g_mods[i].pkg, pkg_name)) {
            g_mods[i].used = 0;
        }
    }
}

static int pkg_add_mod(pkg_entry_t *pe, const char *name, const char *path, const char *desc) {
    for (int i = 0; i < PKG_MOD_MAX; ++i) {
        if (g_mods[i].used && pkg_streq(g_mods[i].name, name)) {
            pkg_copy(g_mods[i].path, sizeof(g_mods[i].path), path);
            pkg_copy(g_mods[i].desc, sizeof(g_mods[i].desc), desc ? desc : "");
            pkg_copy(g_mods[i].pkg, sizeof(g_mods[i].pkg), pe->name);
            return 0;
        }
    }
    for (int i = 0; i < PKG_MOD_MAX; ++i) {
        if (!g_mods[i].used) {
            g_mods[i].used = 1;
            pkg_copy(g_mods[i].name, sizeof(g_mods[i].name), name);
            pkg_copy(g_mods[i].path, sizeof(g_mods[i].path), path);
            pkg_copy(g_mods[i].desc, sizeof(g_mods[i].desc), desc ? desc : "");
            pkg_copy(g_mods[i].pkg, sizeof(g_mods[i].pkg), pe->name);
            return 0;
        }
    }
    return -1;
}

static int pkg_add_file(pkg_entry_t *pe, const char *path) {
    if (pe->nfiles >= PKG_FILE_MAX) {
        return -1;
    }
    for (int i = 0; i < pe->nfiles; ++i) {
        if (pkg_streq(pe->files[i], path)) {
            return 0;
        }
    }
    pkg_copy(pe->files[pe->nfiles], PKG_PATH_MAX, path);
    pe->nfiles++;
    return 0;
}

static int pkg_parse_kv(pkg_entry_t *pe, const char *key, const char *val) {
    if (pkg_streq(key, "name")) {
        pkg_copy(pe->name, sizeof(pe->name), val);
        return 0;
    }
    if (pkg_streq(key, "version")) {
        pkg_copy(pe->version, sizeof(pe->version), val);
        return 0;
    }
    if (pkg_streq(key, "desc") || pkg_streq(key, "description")) {
        pkg_copy(pe->desc, sizeof(pe->desc), val);
        return 0;
    }
    if (pkg_streq(key, "file")) {
        return pkg_add_file(pe, val);
    }
    return 0;
}

static int pkg_parse_mod_line(pkg_entry_t *pe, const char *line) {
    char name[PKG_NAME_MAX];
    char path[PKG_PATH_MAX];
    char desc[48];
    size_t i = 0;
    size_t k = 0;
    while (line[i] && line[i] != ':' && k + 1 < sizeof(name)) {
        name[k++] = line[i++];
    }
    name[k] = '\0';
    if (line[i] != ':') {
        return -1;
    }
    i++;
    k = 0;
    while (line[i] && line[i] != ':' && k + 1 < sizeof(path)) {
        path[k++] = line[i++];
    }
    path[k] = '\0';
    desc[0] = '\0';
    if (line[i] == ':') {
        i++;
        pkg_copy(desc, sizeof(desc), line + i);
    }
    if (name[0] == '\0' || path[0] == '\0') {
        return -1;
    }
    return pkg_add_mod(pe, name, path, desc);
}

static int pkg_parse_manifest_text(const char *text, const char *manifest_path) {
    pkg_entry_t *pe = 0;
    char line[256];
    size_t li = 0;

    pe = pkg_alloc();
    if (!pe) {
        return -1;
    }
    if (manifest_path) {
        pkg_copy(pe->manifest, sizeof(pe->manifest), manifest_path);
        pkg_add_file(pe, manifest_path);
    }

    for (size_t i = 0; text[i] || li > 0; ++i) {
        char c = text[i];
        if (c == '\0' && li == 0) {
            break;
        }
        if (c == '\n' || c == '\r' || c == '\0') {
            line[li] = '\0';
            pkg_trim(line);
            if (line[0] != '\0' && line[0] != '#') {
                if (pkg_prefix(line, "mod=")) {
                    (void)pkg_parse_mod_line(pe, line + 4);
                } else {
                    char *eq = line;
                    while (*eq && *eq != '=') {
                        eq++;
                    }
                    if (*eq == '=') {
                        *eq++ = '\0';
                        (void)pkg_parse_kv(pe, line, eq);
                    }
                }
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

    if (pe->name[0] == '\0') {
        pe->used = 0;
        return -1;
    }

    pkg_entry_t *old = pkg_find(pe->name);
    if (old && old != pe) {
        pkg_clear_mods_for(old->name);
        old->used = 0;
    }

    storage_put("pkg_last", pe->name);
    log_event("PKG installed");
    return 0;
}

int pkg_install_manifest(const char *manifest_path) {
    static char buf[4096];
    size_t got = 0;

    if (!manifest_path || !gfs_present()) {
        return -1;
    }
    if (gfs_read_file(manifest_path, buf, sizeof(buf) - 1, &got) != 0 || got == 0) {
        return -1;
    }
    buf[got] = '\0';
    return pkg_parse_manifest_text(buf, manifest_path);
}

void pkg_rescan(void) {
    char paths[32][PKG_PATH_MAX];
    int n;
    int i;

    for (i = 0; i < PKG_MAX; ++i) {
        g_pkgs[i].used = 0;
    }
    for (i = 0; i < PKG_MOD_MAX; ++i) {
        g_mods[i].used = 0;
    }

    if (!gfs_present()) {
        return;
    }

    n = gfs_list_paths("/packages/", paths, 32);
    for (i = 0; i < n; ++i) {
        if (pkg_ends_manifest(paths[i])) {
            (void)pkg_install_manifest(paths[i]);
        }
    }
}

void pkg_init(void) {
    pkg_rescan();
}

int pkg_remove(const char *name) {
    pkg_entry_t *pe = pkg_find(name);
    if (!pe) {
        return -1;
    }
    for (int i = 0; i < pe->nfiles; ++i) {
        (void)gfs_delete_file(pe->files[i]);
    }
    if (pe->manifest[0]) {
        (void)gfs_delete_file(pe->manifest);
    }
    pkg_clear_mods_for(pe->name);
    pe->used = 0;
    log_event("PKG removed");
    return 0;
}

static int parse_uint(const char *text, uint32_t *out) {
    uint32_t value = 0;
    if (!text || !*text) {
        return -1;
    }
    while (*text >= '0' && *text <= '9') {
        value = (value * 10u) + (uint32_t)(*text - '0');
        text++;
    }
    *out = value;
    return 0;
}

int pkg_recv_gridlink(void) {
    char line[128];
    size_t len;
    int files = 0;
    char last_manifest[PKG_PATH_MAX];

    last_manifest[0] = '\0';

    if (!security_require_capability(CAP_STORAGE, "pkg recv")) {
        return -1;
    }
    if (!gfs_present()) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GFS not mounted — attach Flynn arcade disk.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    console_write_line("GridPKG: waiting for PKG frame on COM1...");

    for (int attempt = 0; attempt < 512; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (pkg_prefix(line, GRIDLINK_PKG)) {
            break;
        }
        if (pkg_prefix(line, GRIDLINK_END)) {
            console_write_line("GridPKG: empty package.");
            return 0;
        }
    }

    for (int attempt = 0; attempt < 256; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (pkg_prefix(line, GRIDLINK_END)) {
            break;
        }
        char path[PKG_PATH_MAX];
        uint32_t size = 0;
        size_t i = 0;
        while (line[i] && line[i] != ' ' && i + 1 < sizeof(path)) {
            path[i] = line[i];
            i++;
        }
        path[i] = '\0';
        if (path[0] == '\0') {
            continue;
        }
        while (line[i] == ' ') {
            i++;
        }
        if (parse_uint(line + i, &size) != 0 || size == 0 || size > 16384) {
            continue;
        }
        uint8_t buf[16384];
        uint32_t got = 0;
        while (got < size) {
            int b = serial_read_byte();
            if (b < 0) {
                break;
            }
            buf[got++] = (uint8_t)b;
        }
        if (got != size) {
            continue;
        }
        if (gfs_write_file(path, buf, size) != 0) {
            continue;
        }
        files++;
        if (pkg_ends_manifest(path)) {
            pkg_copy(last_manifest, sizeof(last_manifest), path);
        }
    }

    if (last_manifest[0]) {
        (void)pkg_install_manifest(last_manifest);
    } else {
        pkg_rescan();
    }

    console_set_color(GRID_COL_OK);
    console_write("GridPKG: installed ");
    console_write_char((char)('0' + (files % 10)));
    console_write_line(" file(s)");
    console_set_color(GRID_COL_DEFAULT);
    return files > 0 ? 0 : -1;
}

void pkg_list_packages(void) {
    int any = 0;
    console_set_color(GRID_COL_TITLE);
    console_write_line("=== Installed Grid packages ===");
    console_set_color(GRID_COL_DEFAULT);
    for (int i = 0; i < PKG_MAX; ++i) {
        if (!g_pkgs[i].used) {
            continue;
        }
        any = 1;
        console_write("  ");
        console_write(g_pkgs[i].name);
        console_write(" v");
        console_write_line(g_pkgs[i].version[0] ? g_pkgs[i].version : "?");
        if (g_pkgs[i].desc[0]) {
            console_write("    ");
            console_write_line(g_pkgs[i].desc);
        }
    }
    if (!any) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (none — pkg recv or install manifest from /packages/)");
        console_set_color(GRID_COL_DEFAULT);
    }
}

void pkg_list_modules(void) {
    int any = 0;
    console_set_color(GRID_COL_TITLE);
    console_write_line("=== GridBASIC IDE modules ===");
    console_set_color(GRID_COL_DEFAULT);
    for (int i = 0; i < PKG_MOD_MAX; ++i) {
        if (!g_mods[i].used) {
            continue;
        }
        any = 1;
        console_write("  ");
        console_write(g_mods[i].name);
        console_write("  [");
        console_write(g_mods[i].pkg);
        console_write("]  ");
        console_write_line(g_mods[i].path);
        if (g_mods[i].desc[0]) {
            console_write("    ");
            console_write_line(g_mods[i].desc);
        }
    }
    if (!any) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (none — install a package with mod= entries)");
        console_set_color(GRID_COL_DEFAULT);
    }
    console_write_line("");
    console_write_line("Run: basic mod run <name>   IDE: Esc :mod run <name>");
    console_write_line("Load: basic mod load <name>  IDE: Esc :mod load <name>");
}

int pkg_info(const char *name) {
    pkg_entry_t *pe = pkg_find(name);
    if (!pe) {
        return -1;
    }
    console_write("Package: ");
    console_write(pe->name);
    console_write(" v");
    console_write_line(pe->version);
    if (pe->desc[0]) {
        console_write_line(pe->desc);
    }
    console_write_line("Files:");
    for (int i = 0; i < pe->nfiles; ++i) {
        console_write("  ");
        console_write_line(pe->files[i]);
    }
    console_write_line("Modules:");
    for (int i = 0; i < PKG_MOD_MAX; ++i) {
        if (g_mods[i].used && pkg_streq(g_mods[i].pkg, pe->name)) {
            console_write("  ");
            console_write(g_mods[i].name);
            console_write(" -> ");
            console_write_line(g_mods[i].path);
        }
    }
    return 0;
}

int pkg_find_module(const char *name, char *path_out, size_t path_cap,
                    char *desc_out, size_t desc_cap) {
    for (int i = 0; i < PKG_MOD_MAX; ++i) {
        if (g_mods[i].used && pkg_streq(g_mods[i].name, name)) {
            if (path_out && path_cap) {
                pkg_copy(path_out, path_cap, g_mods[i].path);
            }
            if (desc_out && desc_cap) {
                pkg_copy(desc_out, desc_cap, g_mods[i].desc);
            }
            return 0;
        }
    }
    return -1;
}

int pkg_run_module(const char *name) {
    char path[PKG_PATH_MAX];
    if (pkg_find_module(name, path, sizeof(path), 0, 0) != 0) {
        return -1;
    }
    return basic_run_file(path);
}

int pkg_load_module_path(const char *path) {
    return basic_ide_load_module(path);
}

void pkg_format_package_list(char *out, size_t cap) {
    size_t pos = 0;
    if (!out || cap == 0) {
        return;
    }
    for (int i = 0; i < PKG_MAX && pos + 1 < cap; ++i) {
        if (!g_pkgs[i].used) {
            continue;
        }
        if (pos > 0 && pos + 1 < cap) {
            out[pos++] = ',';
        }
        const char *s = g_pkgs[i].name;
        while (*s && pos + 1 < cap) {
            out[pos++] = *s++;
        }
    }
    out[pos] = '\0';
}

void pkg_format_module_list(char *out, size_t cap) {
    size_t pos = 0;
    if (!out || cap == 0) {
        return;
    }
    for (int i = 0; i < PKG_MOD_MAX && pos + 1 < cap; ++i) {
        if (!g_mods[i].used) {
            continue;
        }
        if (pos > 0 && pos + 1 < cap) {
            out[pos++] = ',';
        }
        const char *s = g_mods[i].name;
        while (*s && pos + 1 < cap) {
            out[pos++] = *s++;
        }
    }
    out[pos] = '\0';
}
