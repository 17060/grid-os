#include "console.h"
#include "gfs.h"
#include "link.h"
#include "log.h"
#include "security.h"
#include "serial.h"
#include "storage.h"

#include <stddef.h>
#include <stdint.h>

#define GRIDLINK_HDR "GRIDLINK/1.0"
#define GRIDLINK_VAULT GRIDLINK_HDR "/VAULT"
#define GRIDLINK_FILE GRIDLINK_HDR "/FILE"
#define GRIDLINK_END "#GRIDLINK/END"

static int equals_prefix(const char *line, const char *prefix) {
    while (*prefix) {
        if (*line++ != *prefix++) {
            return 0;
        }
    }
    return 1;
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

void gridlink_export_vault(void) {
    serial_write(GRIDLINK_HDR);
    serial_write("/VAULT\n");
    storage_export_serial();
    serial_write(GRIDLINK_END);
    serial_write("\n");
}

int gridlink_import_vault(void) {
    char line[128];
    size_t len;

    console_write_line("GridLink: waiting for vault frame on COM1...");
    console_write_line("(Host: gridctl portal pull, or paste export ending with END)");

    for (int attempt = 0; attempt < 256; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (equals_prefix(line, GRIDLINK_VAULT)) {
            break;
        }
    }

    if (storage_import_serial() != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GridLink vault import failed.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    console_set_color(GRID_COL_OK);
    console_write_line("GridLink vault imported.");
    console_set_color(GRID_COL_DEFAULT);
    return 0;
}

int gridlink_recv_file(void) {
    char line[128];
    char path[GFS_PATH_MAX];
    uint8_t buffer[16384];
    uint32_t expected = 0;
    uint32_t received = 0;
    size_t len;
    int got_header = 0;

    if (!security_require_capability(CAP_STORAGE, "portal recv")) {
        return -1;
    }
    if (!gfs_present()) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GFS not mounted — attach Flynn arcade disk.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    console_write_line("GridLink: waiting for FILE frame on COM1...");

    for (int attempt = 0; attempt < 512 && !got_header; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (!equals_prefix(line, GRIDLINK_FILE)) {
            continue;
        }

        size_t path_start = sizeof(GRIDLINK_FILE) - 1;
        size_t i = 0;
        while (line[path_start + i] && i + 1 < sizeof(path)) {
            path[i] = line[path_start + i];
            i++;
        }
        path[i] = '\0';
        if (path[0] != '/') {
            console_set_color(GRID_COL_ERROR);
            console_write_line("GridLink: path must be absolute (/programs/name).");
            console_set_color(GRID_COL_DEFAULT);
            return -1;
        }
        got_header = 1;
    }

    if (!got_header) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GridLink: timed out waiting for FILE header.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    for (int attempt = 0; attempt < 256; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (parse_uint(line, &expected) == 0) {
            break;
        }
    }

    if (expected == 0 || expected > sizeof(buffer)) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GridLink: invalid file size.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    while (received < expected) {
        if (!serial_can_read()) {
            continue;
        }
        int byte = serial_read_byte();
        if (byte < 0) {
            continue;
        }
        buffer[received++] = (uint8_t)byte;
    }

    for (int attempt = 0; attempt < 64; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (equals_prefix(line, GRIDLINK_END)) {
            break;
        }
    }

    if (gfs_write_file(path, buffer, expected) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GridLink: GFS write failed.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    console_set_color(GRID_COL_OK);
    console_write("GridLink: installed ");
    console_write(path);
    console_write(" (");
    char size_buf[16];
    size_t pos = 0;
    uint32_t count = expected;
    if (count == 0) {
        size_buf[pos++] = '0';
    } else {
        char tmp[16];
        size_t tlen = 0;
        while (count > 0) {
            tmp[tlen++] = (char)('0' + (count % 10));
            count /= 10;
        }
        while (tlen > 0) {
            size_buf[pos++] = tmp[--tlen];
        }
    }
    size_buf[pos] = '\0';
    console_write(size_buf);
    console_write_line(" B)");
    console_set_color(GRID_COL_DEFAULT);
    return 0;
}

#define GRIDLINK_PKG GRIDLINK_HDR "/PKG"

int gridlink_recv_package(void) {
    char line[128];
    size_t len;
    int files = 0;

    if (!security_require_capability(CAP_STORAGE, "portal pkg")) {
        return -1;
    }
    if (!gfs_present()) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GFS not mounted — attach Flynn arcade disk.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    console_write_line("GridLink: waiting for PKG frame on COM1...");

    for (int attempt = 0; attempt < 512; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (equals_prefix(line, GRIDLINK_PKG)) {
            break;
        }
        if (equals_prefix(line, GRIDLINK_END)) {
            console_write_line("GridLink: empty package.");
            return 0;
        }
    }

    for (int attempt = 0; attempt < 256; ++attempt) {
        len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }
        if (equals_prefix(line, GRIDLINK_END)) {
            break;
        }
        char path[GFS_PATH_MAX];
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
        if (gfs_write_file(path, buf, size) == 0) {
            files++;
        }
    }

    console_set_color(GRID_COL_OK);
    console_write("GridLink: package installed ");
    console_write_char((char)('0' + (files % 10)));
    console_write_line(" file(s)");
    console_set_color(GRID_COL_DEFAULT);
    return files > 0 ? 0 : -1;
}

int gridlink_duel_ping(void) {
    serial_write(GRIDLINK_HDR);
    serial_write("/DUEL\n");
    serial_write("lightcycle\n");
    serial_write(GRIDLINK_END);
    serial_write("\n");
    log_event("GridLink duel ping sent");
    return 0;
}
