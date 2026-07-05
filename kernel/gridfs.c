#include "console.h"
#include "gfs.h"
#include "gridfs.h"
#include "iso.h"
#include "security.h"
#include "storage.h"

#include <stddef.h>
#include <stdint.h>

static int starts_with(const char *text, const char *prefix) {
    while (*prefix) {
        if (*text++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static int equals(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static void copy_string(char *dest, size_t dest_len, const char *src) {
    size_t i = 0;
    if (src) {
        while (src[i] && i + 1 < dest_len) {
            dest[i] = src[i];
            i++;
        }
    }
    dest[i] = '\0';
}

int gridfs_read(const char *path, char *out, size_t out_len) {
    char disc[GRID_DISC_HEX];
    char cycles[16];
    char buffer[512];
    size_t file_len = 0;

    if (!path || !out || out_len == 0) {
        return -1;
    }

    if (equals(path, "/") || equals(path, "")) {
        copy_string(out, out_len, "(directory)");
        return 0;
    }

    if (equals(path, "/disc")) {
        security_format_disc(disc, sizeof(disc));
        copy_string(out, out_len, disc);
        return 0;
    }

    if (equals(path, "/cycles")) {
        uint32_t value = security_cycles();
        size_t pos = 0;
        if (value == 0) {
            cycles[pos++] = '0';
        } else {
            char tmp[16];
            size_t tlen = 0;
            while (value > 0) {
                tmp[tlen++] = (char)('0' + (value % 10));
                value /= 10;
            }
            while (tlen > 0) {
                cycles[pos++] = tmp[--tlen];
            }
        }
        cycles[pos] = '\0';
        copy_string(out, out_len, cycles);
        return 0;
    }

    if (gfs_present()) {
        if (gfs_read_file(path, buffer, sizeof(buffer) - 1, &file_len) == 0) {
            buffer[file_len] = '\0';
            copy_string(out, out_len, buffer);
            return 0;
        }
    }

    if (path[0] == '/' && path[1] != '\0' && !starts_with(path, "/vault/") &&
        !starts_with(path, "/isos") && !starts_with(path, "/programs") &&
        !starts_with(path, "/flynn") && !starts_with(path, "/grid") &&
        !starts_with(path, "/source")) {
        const char *key = path + 1;
        const char *value = storage_get(key);
        if (value) {
            copy_string(out, out_len, value);
            return 0;
        }
    }

    if (starts_with(path, "/vault/")) {
        const char *key = path + 7;
        const char *value = storage_get(key);
        if (value) {
            copy_string(out, out_len, value);
            return 0;
        }
        return -1;
    }

    if (equals(path, "/isos")) {
        copy_string(out, out_len, "(iso directory)");
        return 0;
    }

    if (starts_with(path, "/isos/")) {
        int id = path[6] - '0';
        if (id < 1 || id > ISO_ZONE_SLOTS) {
            return -1;
        }

        if (equals(path + 7, "/name")) {
            const iso_entity_t *iso = iso_get(id);
            if (!iso) {
                return -1;
            }
            copy_string(out, out_len, iso->name);
            return 0;
        }

        if (equals(path + 7, "/genome")) {
            iso_format_genome(id, buffer, sizeof(buffer));
            copy_string(out, out_len, buffer);
            return 0;
        }

        if (equals(path + 7, "/disc")) {
            iso_format_disc(id, buffer, sizeof(buffer));
            copy_string(out, out_len, buffer);
            return 0;
        }
    }

    return -1;
}

int gridfs_copy_to_user(const char *path, char *out, size_t out_len) {
    return gridfs_read(path, out, out_len);
}

void gridfs_list(const char *path) {
    if (!path || equals(path, "/") || equals(path, "")) {
        console_write_line("GridFS /");
        console_write_line("  disc              identity disc (virtual)");
        console_write_line("  cycles            grid time (virtual)");
        console_write_line("  vault/            persistent vault keys");
        console_write_line("  isos/             ISO research zone");
        if (gfs_present()) {
            console_write_line("  flynn/            Flynn's archive (disk)");
            console_write_line("  source/           Grid Workshop programs (disk)");
            console_write_line("  programs/         ring-3 binaries (disk)");
            console_write_line("  grid/             system logs (disk)");
        }
        return;
    }

    if (equals(path, "/programs") || starts_with(path, "/programs/")) {
        console_write_line("GridFS /programs (on-disk):");
        gfs_list("/programs/");
        return;
    }

    if (equals(path, "/flynn") || starts_with(path, "/flynn/")) {
        console_write_line("GridFS /flynn (on-disk):");
        gfs_list("/flynn/");
        return;
    }

    if (equals(path, "/source") || starts_with(path, "/source/")) {
        console_write_line("GridFS /source (on-disk):");
        gfs_list("/source/");
        return;
    }

    if (equals(path, "/grid") || starts_with(path, "/grid/")) {
        console_write_line("GridFS /grid (on-disk):");
        gfs_list("/grid/");
        return;
    }

    if (equals(path, "/vault") || starts_with(path, "/vault/")) {
        storage_list();
        return;
    }

    if (equals(path, "/isos")) {
        iso_print_list();
        return;
    }

    console_set_color(GRID_COL_ERROR);
    console_write_line("GridFS path not found.");
    console_set_color(GRID_COL_DEFAULT);
}
