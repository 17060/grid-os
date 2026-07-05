#include "console.h"
#include "disk.h"
#include "gfs.h"

#include <stddef.h>
#include <stdint.h>

#define GFS_MAGIC0 'G'
#define GFS_MAGIC1 'F'
#define GFS_MAGIC2 'S'
#define GFS_MAGIC3 '2'
#define GFS_MAGIC4 'F'
#define GFS_MAGIC5 'L'
#define GFS_MAGIC6 'Y'
#define GFS_MAGIC7 'N'

#define GFS_VERSION 2u
#define GFS_SUPER_LBA 64u
#define GFS_INODE_TABLE_LBA 65u
#define GFS_INODE_TABLE_SECTORS 8u
#define GFS_INODE_MAX 64u
#define GFS_SECTORS_PER_FILE 32u
#define GFS_DATA_BASE_LBA 128u
#define GFS_FILE_CAP (GFS_SECTORS_PER_FILE * 512u)

#define GFS_INODE_MAGIC 0x46494C45u

typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t inode_count;
    uint32_t data_base_lba;
    uint32_t sectors_per_file;
    uint32_t checksum;
} gfs_super_t;

typedef struct {
    uint32_t valid;
    uint32_t size;
    char path[GFS_PATH_MAX];
} gfs_inode_t;

static int mounted = 0;
static gfs_inode_t inodes[GFS_INODE_MAX];

extern const uint8_t gridprog_bin[];
extern const uint8_t gridprog_bin_end[];
extern const uint8_t discinfo_bin[];
extern const uint8_t discinfo_bin_end[];
extern const uint8_t gridsh_bin[];
extern const uint8_t gridsh_bin_end[];
extern const uint8_t lightcycle_bin[];
extern const uint8_t lightcycle_bin_end[];
extern const uint8_t gridloop_bin[];
extern const uint8_t gridloop_bin_end[];

static int path_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static int path_starts_with(const char *path, const char *prefix) {
    while (*prefix) {
        if (*path++ != *prefix++) {
            return 0;
        }
    }
    return 1;
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

static uint32_t crc32_update(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i) {
        if (crc & 1u) {
            crc = (crc >> 1) ^ 0xEDB88320u;
        } else {
            crc >>= 1;
        }
    }
    return crc;
}

static uint32_t gfs_super_checksum(const gfs_super_t *sb) {
    const uint8_t *bytes = (const uint8_t *)sb;
    uint32_t crc = 0xFFFFFFFFu;
    size_t limit = sizeof(gfs_super_t) - sizeof(uint32_t);

    for (size_t i = 0; i < limit; ++i) {
        crc = crc32_update(crc, bytes[i]);
    }
    return crc ^ 0xFFFFFFFFu;
}

static int super_valid(const gfs_super_t *sb) {
    if (sb->magic[0] != GFS_MAGIC0 || sb->magic[1] != GFS_MAGIC1 ||
        sb->magic[2] != GFS_MAGIC2 || sb->magic[3] != GFS_MAGIC3 ||
        sb->magic[4] != GFS_MAGIC4 || sb->magic[5] != GFS_MAGIC5 ||
        sb->magic[6] != GFS_MAGIC6 || sb->magic[7] != GFS_MAGIC7) {
        return 0;
    }
    if (sb->version != GFS_VERSION) {
        return 0;
    }
    return sb->checksum == gfs_super_checksum(sb);
}

static void build_superblock(gfs_super_t *sb) {
    sb->magic[0] = GFS_MAGIC0;
    sb->magic[1] = GFS_MAGIC1;
    sb->magic[2] = GFS_MAGIC2;
    sb->magic[3] = GFS_MAGIC3;
    sb->magic[4] = GFS_MAGIC4;
    sb->magic[5] = GFS_MAGIC5;
    sb->magic[6] = GFS_MAGIC6;
    sb->magic[7] = GFS_MAGIC7;
    sb->version = GFS_VERSION;
    sb->inode_count = GFS_INODE_MAX;
    sb->data_base_lba = GFS_DATA_BASE_LBA;
    sb->sectors_per_file = GFS_SECTORS_PER_FILE;
    sb->checksum = 0;
    sb->checksum = gfs_super_checksum(sb);
}

static uint32_t inode_data_lba(int slot) {
    return GFS_DATA_BASE_LBA + (uint32_t)slot * GFS_SECTORS_PER_FILE;
}

static int load_inodes(void) {
    uint8_t sector[512];

    for (uint32_t sector_idx = 0; sector_idx < GFS_INODE_TABLE_SECTORS; ++sector_idx) {
        if (disk_read(GFS_INODE_TABLE_LBA + sector_idx, sector) != 0) {
            return -1;
        }

        gfs_inode_t *chunk = (gfs_inode_t *)sector;
        for (size_t i = 0; i < 512 / sizeof(gfs_inode_t); ++i) {
            size_t global = sector_idx * (512 / sizeof(gfs_inode_t)) + i;
            if (global >= GFS_INODE_MAX) {
                return 0;
            }
            inodes[global] = chunk[i];
        }
    }

    return 0;
}

static int store_inodes(void) {
    uint8_t sector[512];

    for (uint32_t sector_idx = 0; sector_idx < GFS_INODE_TABLE_SECTORS; ++sector_idx) {
        for (size_t i = 0; i < 512 / sizeof(gfs_inode_t); ++i) {
            size_t global = sector_idx * (512 / sizeof(gfs_inode_t)) + i;
            gfs_inode_t *chunk = (gfs_inode_t *)sector;
            if (global < GFS_INODE_MAX) {
                chunk[i] = inodes[global];
            } else {
                chunk[i].valid = 0;
                chunk[i].size = 0;
                chunk[i].path[0] = '\0';
            }
        }

        if (disk_write(GFS_INODE_TABLE_LBA + sector_idx, sector) != 0) {
            return -1;
        }
    }

    return 0;
}

static int find_inode(const char *path) {
    for (int i = 0; i < (int)GFS_INODE_MAX; ++i) {
        if (inodes[i].valid == GFS_INODE_MAGIC && path_equal(inodes[i].path, path)) {
            return i;
        }
    }
    return -1;
}

static int find_free_inode(void) {
    for (int i = 1; i < (int)GFS_INODE_MAX; ++i) {
        if (inodes[i].valid != GFS_INODE_MAGIC) {
            return i;
        }
    }
    return -1;
}

int gfs_present(void) {
    return mounted;
}

int gfs_format(void) {
    gfs_super_t sb;
    uint8_t zero[512];

    if (!disk_present()) {
        return -1;
    }

    for (size_t i = 0; i < sizeof(zero); ++i) {
        zero[i] = 0;
    }

    build_superblock(&sb);
    if (disk_write(GFS_SUPER_LBA, &sb) != 0) {
        return -1;
    }

    for (int i = 0; i < (int)GFS_INODE_MAX; ++i) {
        inodes[i].valid = 0;
        inodes[i].size = 0;
        inodes[i].path[0] = '\0';
    }

    if (store_inodes() != 0) {
        return -1;
    }

    for (uint32_t slot = 0; slot < GFS_INODE_MAX; ++slot) {
        uint32_t base = inode_data_lba((int)slot);
        for (uint32_t s = 0; s < GFS_SECTORS_PER_FILE; ++s) {
            if (disk_write(base + s, zero) != 0) {
                return -1;
            }
        }
    }

    mounted = 1;
    return 0;
}

void gfs_init(void) {
    gfs_super_t sb;

    mounted = 0;
    for (int i = 0; i < (int)GFS_INODE_MAX; ++i) {
        inodes[i].valid = 0;
        inodes[i].size = 0;
        inodes[i].path[0] = '\0';
    }

    if (!disk_present()) {
        return;
    }

    if (disk_read(GFS_SUPER_LBA, &sb) != 0) {
        return;
    }

    if (!super_valid(&sb)) {
        if (gfs_format() != 0) {
            return;
        }
        gfs_seed_defaults();
        return;
    }

    if (load_inodes() != 0) {
        return;
    }

    mounted = 1;
}

int gfs_read_file(const char *path, void *out, size_t out_cap, size_t *out_len) {
    int slot;
    uint32_t lba;
    size_t total = 0;
    uint8_t sector[512];

    if (!mounted || !path || !out) {
        return -1;
    }

    slot = find_inode(path);
    if (slot < 0) {
        return -1;
    }

    if (inodes[slot].size > out_cap) {
        return -1;
    }

    lba = inode_data_lba(slot);
    while (total < inodes[slot].size) {
        if (disk_read(lba, sector) != 0) {
            return -1;
        }

        size_t chunk = inodes[slot].size - total;
        if (chunk > 512) {
            chunk = 512;
        }

        for (size_t i = 0; i < chunk; ++i) {
            ((uint8_t *)out)[total + i] = sector[i];
        }

        total += chunk;
        lba++;
    }

    if (out_len) {
        *out_len = total;
    }
    return 0;
}

int gfs_write_file(const char *path, const void *data, size_t size) {
    int slot;
    uint32_t lba;
    size_t written = 0;
    uint8_t sector[512];
    const uint8_t *bytes = (const uint8_t *)data;

    if (!mounted || !path || !data || size == 0 || size > GFS_FILE_CAP) {
        return -1;
    }

    slot = find_inode(path);
    if (slot < 0) {
        slot = find_free_inode();
        if (slot < 0) {
            return -1;
        }
        inodes[slot].valid = GFS_INODE_MAGIC;
        copy_string(inodes[slot].path, sizeof(inodes[slot].path), path);
    }

    inodes[slot].size = (uint32_t)size;
    lba = inode_data_lba(slot);

    while (written < size) {
        size_t chunk = size - written;
        if (chunk > 512) {
            chunk = 512;
        }

        for (size_t i = 0; i < 512; ++i) {
            sector[i] = (i < chunk) ? bytes[written + i] : 0;
        }

        if (disk_write(lba, sector) != 0) {
            return -1;
        }

        written += chunk;
        lba++;
    }

    return store_inodes();
}

int gfs_delete_file(const char *path) {
    int slot;

    if (!mounted || !path) {
        return -1;
    }

    slot = find_inode(path);
    if (slot < 0) {
        return -1;
    }

    inodes[slot].valid = 0;
    inodes[slot].size = 0;
    inodes[slot].path[0] = '\0';
    return store_inodes();
}

void gfs_list(const char *prefix) {
    int found = 0;

    if (!mounted) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GFS: arcade disk not mounted.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    if (!prefix || prefix[0] == '\0' || path_equal(prefix, "/")) {
        prefix = "";
    }

    for (int i = 0; i < (int)GFS_INODE_MAX; ++i) {
        if (inodes[i].valid != GFS_INODE_MAGIC) {
            continue;
        }
        if (prefix[0] != '\0' && !path_starts_with(inodes[i].path, prefix)) {
            continue;
        }

        found = 1;
        console_write("  ");
        console_set_color(GRID_COL_OK);
        console_write(inodes[i].path);
        console_set_color(GRID_COL_DEFAULT);
        console_write("  (");
        char size_buf[16];
        size_t pos = 0;
        uint32_t sz = inodes[i].size;
        if (sz == 0) {
            size_buf[pos++] = '0';
        } else {
            char tmp[16];
            size_t tlen = 0;
            while (sz > 0) {
                tmp[tlen++] = (char)('0' + (sz % 10));
                sz /= 10;
            }
            while (tlen > 0) {
                size_buf[pos++] = tmp[--tlen];
            }
        }
        size_buf[pos++] = ' ';
        size_buf[pos++] = 'B';
        size_buf[pos] = '\0';
        console_write(size_buf);
        console_write_line(")");
    }

    if (!found) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (empty)");
        console_set_color(GRID_COL_DEFAULT);
    }
}

int gfs_list_paths(const char *prefix, char paths[][GFS_PATH_MAX], int max_paths) {
    int count = 0;

    if (!mounted || max_paths <= 0) {
        return 0;
    }

    if (!prefix || prefix[0] == '\0' || path_equal(prefix, "/")) {
        prefix = "";
    }

    for (int i = 0; i < (int)GFS_INODE_MAX && count < max_paths; ++i) {
        if (inodes[i].valid != GFS_INODE_MAGIC) {
            continue;
        }
        if (prefix[0] != '\0' && !path_starts_with(inodes[i].path, prefix)) {
            continue;
        }
        copy_string(paths[count], GFS_PATH_MAX, inodes[i].path);
        count++;
    }

    return count;
}

void gfs_print_status(void) {
    int used = 0;

    if (!mounted) {
        console_write_line("GFS: not mounted (no Flynn arcade disk)");
        return;
    }

    for (int i = 0; i < (int)GFS_INODE_MAX; ++i) {
        if (inodes[i].valid == GFS_INODE_MAGIC) {
            used++;
        }
    }

    console_write_line("Flynn GridFS (on-disk):");
    console_set_color(GRID_COL_OK);
    console_write("  Magic:     GFS2FLYN v");
    console_write_char((char)('0' + GFS_VERSION));
    console_write_line("");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Inodes:    ");
    char buf[16];
    size_t pos = 0;
    uint32_t count = (uint32_t)used;
    if (count == 0) {
        buf[pos++] = '0';
    } else {
        char tmp[16];
        size_t tlen = 0;
        while (count > 0) {
            tmp[tlen++] = (char)('0' + (count % 10));
            count /= 10;
        }
        while (tlen > 0) {
            buf[pos++] = tmp[--tlen];
        }
    }
    buf[pos] = '\0';
    console_write(buf);
    console_write(" / ");
    console_write_char((char)('0' + (GFS_INODE_MAX / 10) % 10));
    console_write_char((char)('0' + (GFS_INODE_MAX % 10)));
    console_write_line(" files");
    console_write_line("  Max file:  16384 B");
    console_write_line("  Paths: /programs/*  /source/*  /flynn/*  /grid/*");
}

static int seed_one(const char *path, const void *data, size_t size) {
    if (find_inode(path) >= 0) {
        return 0;
    }
    return gfs_write_file(path, data, size);
}

int gfs_seed_defaults(void) {
    size_t size;

    if (!mounted) {
        return -1;
    }

    seed_one("/flynn/motd", "The Grid is open. Flynn's archive linked.\n", 42);

    size = (size_t)(gridsh_bin_end - gridsh_bin);
    seed_one("/programs/gridsh", gridsh_bin, size);

    size = (size_t)(discinfo_bin_end - discinfo_bin);
    seed_one("/programs/discinfo", discinfo_bin, size);

    size = (size_t)(gridprog_bin_end - gridprog_bin);
    seed_one("/programs/gridprog", gridprog_bin, size);

    size = (size_t)(lightcycle_bin_end - lightcycle_bin);
    seed_one("/programs/lightcycle", lightcycle_bin, size);

    size = (size_t)(gridloop_bin_end - gridloop_bin);
    seed_one("/programs/gridloop", gridloop_bin, size);

    seed_one("/grid/recognizer.log", "Recognizer patrol: sector clear. End of line.\n", 46);

    seed_one("/source/welcome.grid",
             "10 REM Flynn's Grid Workshop\n"
             "20 PRINT \"========================\"\n"
             "30 PRINT \" GRID WORKSHOP READY.\"\n"
             "40 PRINT \" Flynn's frontier awaits.\"\n"
             "50 PRINT \"========================\"\n"
             "60 CYCLES\n"
             "70 END\n",
             188);

    seed_one("/programs/hello.bas",
             "10 REM GridBASIC demo — the Grid counts\n"
             "20 PRINT \"GridBASIC 6.0 online\"\n"
             "30 FOR I = 1 TO 5\n"
             "40   PRINT \"grid line \"; I\n"
             "50 NEXT I\n"
             "60 S$ = \"hello \" + \"grid\"\n"
             "70 PRINT S$\n"
             "80 PRINT \"ping gw: \"; GRID.PING(\"10.0.2.2\")\n"
             "90 PRINT \"ticks: \"; GRID.TIME\n"
             "100 END\n",
             220);

    return 0;
}
