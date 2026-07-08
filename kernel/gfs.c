#include "console.h"
#include "disk.h"
#include "dns.h"
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

#define GFS_VERSION 3u
#define GFS_SUPER_LBA 64u
#define GFS_INODE_TABLE_LBA 65u
#define GFS_INODE_TABLE_SECTORS 32u
#define GFS_INODE_MAX 256u
#define GFS_SECTORS_PER_FILE 128u
#define GFS_DATA_BASE_LBA 128u
#define GFS_FILE_CAP GFS_FILE_MAX

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

static void gfs_try_mount(void) {
    if (mounted) {
        return;
    }
    if (!disk_present()) {
        return;
    }
    gfs_init();
}

int gfs_read_file(const char *path, void *out, size_t out_cap, size_t *out_len) {
    int slot;
    uint32_t lba;
    size_t total = 0;
    uint8_t sector[512];

    gfs_try_mount();
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

    gfs_try_mount();
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

    if (path[0] == '/' && path[1] == 'e' && path[2] == 't' && path[3] == 'c' &&
        path[4] == '/' && path[5] == 'h' && path[6] == 'o' && path[7] == 's' &&
        path[8] == 't' && path[9] == 's' && path[10] == '\0') {
        hosts_reload();
    }

    return store_inodes();
}

int gfs_delete_file(const char *path) {
    int slot;

    gfs_try_mount();
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

    gfs_try_mount();
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

    gfs_try_mount();
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

    gfs_try_mount();
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
    console_write_line("  Max file:  65536 B");
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
             "20 PRINT \"GridBASIC 7.0 online\"\n"
             "30 FOR I = 1 TO 5\n"
             "40   PRINT \"grid line \"; I\n"
             "50 NEXT I\n"
             "60 S$ = \"hello \" + \"grid\"\n"
             "70 PRINT S$\n"
             "80 PRINT \"ping gw: \"; GRID.PING(\"10.0.2.2\")\n"
             "90 PRINT \"ticks: \"; GRID.TIME\n"
             "100 END\n",
             249);

    seed_one("/programs/autoexec.bas",
             "10 REM Flynn Boot — runs once at Grid OS startup\n"
             "20 PRINT \"\"\n"
             "30 PRINT \"=== Welcome to Flynn's Grid ===\"\n"
             "40 PRINT GRID.STATUS$\n"
             "50 PRINT \"Type 'tutorial' or Esc :load tutorial in IDE\"\n"
             "60 PRINT \"Samples: samples   Modules: pkg mods\"\n"
             "70 PRINT \"Disable boot script: vault put autoexec off\"\n"
             "80 PRINT \"\"\n"
             "90 END\n",
             304);

    seed_one("/programs/tutorial.bas",
             "10 REM GridBASIC Tutorial — Flynn Boot Experience\n"
             "20 GRID.CLS\n"
             "30 PRINT \"=== GridBASIC Tutorial ===\"\n"
             "40 PRINT \"1. PRINT shows text on the grid\"\n"
             "50 PRINT \"2. LET stores numbers in variables\"\n"
             "60 LET N = 42\n"
             "70 PRINT \"   N = \"; N\n"
             "80 PRINT \"3. FOR loops count for you\"\n"
             "90 FOR I = 1 TO 3\n"
             "100   PRINT \"   line \"; I\n"
             "110 NEXT I\n"
             "120 PRINT \"4. Strings join with +\"\n"
             "130 S$ = \"hello \" + \"grid\"\n"
             "140 PRINT \"   \"; S$\n"
             "150 PRINT \"5. GRID.WHOAMI$ = \"; GRID.WHOAMI$\n"
             "160 PRINT \"Try: samples   basic run /programs/subdemo.bas\"\n"
             "170 PRINT \"End of line.\"\n"
             "180 END\n",
             539);

    seed_one("/programs/subdemo.bas",
             "10 REM SUB / FUNCTION demo\n"
             "20 SUB GREET(N$)\n"
             "30   PRINT \"Hello, \"; N$\n"
             "40 END SUB\n"
             "50 FUNCTION DOUBLE(X)\n"
             "60   DOUBLE = X * 2\n"
             "70 END FUNCTION\n"
             "80 CALL GREET(\"Flynn\")\n"
             "90 PRINT \"DOUBLE(21) = \"; DOUBLE(21)\n"
             "100 END\n",
             206);

    seed_one("/programs/grid2d.bas",
             "10 REM 2D array demo\n"
             "20 DIM M(3,3)\n"
             "30 FOR R = 0 TO 3\n"
             "40   FOR C = 0 TO 3\n"
             "50     M(R,C) = R * 10 + C\n"
             "60   NEXT C\n"
             "70 NEXT R\n"
             "80 PRINT \"M(2,3) = \"; M(2,3)\n"
             "90 END\n",
             158);

    seed_one("/programs/btc-demo.bas",
             "10 REM Bitcoin demo -- host: make btc-bridge (testnet/regtest)\n"
             "20 PRINT \"=== Grid BTC ===\"\n"
             "30 PRINT GRID.BTC.STATUS$\n"
             "40 PRINT \"Balance: \"; GRID.BTC.BALANCE$\n"
             "50 GRID.BTC.PRINT \"getblockchaininfo\"\n"
             "60 END\n",
             202);

    seed_one("/packages/flynn-ide-tools/MANIFEST",
             "name=flynn-ide-tools\n"
             "version=2.1\n"
             "desc=25 GridBASIC IDE tools for Flynn's Grid (7.1.1 categories)\n"
             "file=/packages/flynn-ide-tools/MANIFEST\n"
             "file=/packages/flynn-ide-tools/modules/disc-status.bas\n"
             "file=/packages/flynn-ide-tools/modules/grid-ping.bas\n"
             "file=/packages/flynn-ide-tools/modules/patrol-arm.bas\n"
             "file=/packages/flynn-ide-tools/modules/patrol-stand-down.bas\n"
             "file=/packages/flynn-ide-tools/modules/whoami-panel.bas\n"
             "file=/packages/flynn-ide-tools/modules/caps-panel.bas\n"
             "file=/packages/flynn-ide-tools/modules/net-status.bas\n"
             "file=/packages/flynn-ide-tools/modules/dns-lookup.bas\n"
             "file=/packages/flynn-ide-tools/modules/vault-nodes.bas\n"
             "file=/packages/flynn-ide-tools/modules/gfs-programs.bas\n"
             "file=/packages/flynn-ide-tools/modules/jobs-monitor.bas\n"
             "file=/packages/flynn-ide-tools/modules/iso-roster.bas\n"
             "file=/packages/flynn-ide-tools/modules/audit-tail.bas\n"
             "file=/packages/flynn-ide-tools/modules/grid-clock.bas\n"
             "file=/packages/flynn-ide-tools/modules/grid-clear.bas\n"
             "file=/packages/flynn-ide-tools/modules/pkg-index.bas\n"
             "file=/packages/flynn-ide-tools/modules/sample-menu.bas\n"
             "file=/packages/flynn-ide-tools/modules/ide-cheatsheet.bas\n"
             "file=/packages/flynn-ide-tools/modules/beep-scale.bas\n"
             "file=/packages/flynn-ide-tools/modules/plot-grid.bas\n"
             "file=/packages/flynn-ide-tools/modules/ai-ask.bas\n"
             "file=/packages/flynn-ide-tools/modules/btc-snapshot.bas\n"
             "file=/packages/flynn-ide-tools/modules/irc-check.bas\n"
             "file=/packages/flynn-ide-tools/modules/hosts-table.bas\n"
             "file=/packages/flynn-ide-tools/modules/spawn-catalog.bas\n"
             "mod=disc-status:/packages/flynn-ide-tools/modules/disc-status.bas:Identity disc status panel:disc\n"
             "mod=grid-ping:/packages/flynn-ide-tools/modules/grid-ping.bas:Ping gateway and grid hosts:network\n"
             "mod=patrol-arm:/packages/flynn-ide-tools/modules/patrol-arm.bas:Start recognizer patrol:patrol\n"
             "mod=patrol-stand-down:/packages/flynn-ide-tools/modules/patrol-stand-down.bas:Stop recognizer patrol:patrol\n"
             "mod=whoami-panel:/packages/flynn-ide-tools/modules/whoami-panel.bas:Entity type and identity:disc\n"
             "mod=caps-panel:/packages/flynn-ide-tools/modules/caps-panel.bas:Granted capability mask:system\n"
             "mod=net-status:/packages/flynn-ide-tools/modules/net-status.bas:Virtio-net link status:network\n"
             "mod=dns-lookup:/packages/flynn-ide-tools/modules/dns-lookup.bas:Resolve Flynn host names:network\n"
             "mod=vault-nodes:/packages/flynn-ide-tools/modules/vault-nodes.bas:List vault key nodes:storage\n"
             "mod=gfs-programs:/packages/flynn-ide-tools/modules/gfs-programs.bas:List Flynn /programs archive:storage\n"
             "mod=jobs-monitor:/packages/flynn-ide-tools/modules/jobs-monitor.bas:Background sandbox jobs:system\n"
             "mod=iso-roster:/packages/flynn-ide-tools/modules/iso-roster.bas:ISO research zone entities:system\n"
             "mod=audit-tail:/packages/flynn-ide-tools/modules/audit-tail.bas:Recent audit log entries:system\n"
             "mod=grid-clock:/packages/flynn-ide-tools/modules/grid-clock.bas:Grid cycle timer ticks:grid\n"
             "mod=grid-clear:/packages/flynn-ide-tools/modules/grid-clear.bas:Clear screen with Flynn banner:grid\n"
             "mod=pkg-index:/packages/flynn-ide-tools/modules/pkg-index.bas:Installed packages and modules:storage\n"
             "mod=sample-menu:/packages/flynn-ide-tools/modules/sample-menu.bas:GridBASIC sample program guide:dev\n"
             "mod=ide-cheatsheet:/packages/flynn-ide-tools/modules/ide-cheatsheet.bas:IDE colon-command reference:dev\n"
             "mod=beep-scale:/packages/flynn-ide-tools/modules/beep-scale.bas:PC speaker note demo:grid\n"
             "mod=plot-grid:/packages/flynn-ide-tools/modules/plot-grid.bas:VGA plot pattern demo:grid\n"
             "mod=ai-ask:/packages/flynn-ide-tools/modules/ai-ask.bas:Quick AI bridge question:bridge\n"
             "mod=btc-snapshot:/packages/flynn-ide-tools/modules/btc-snapshot.bas:Bitcoin bridge status:bridge\n"
             "mod=irc-check:/packages/flynn-ide-tools/modules/irc-check.bas:IRC session status:network\n"
             "mod=hosts-table:/packages/flynn-ide-tools/modules/hosts-table.bas:Show /etc/hosts from Flynn disk:network\n"
             "mod=spawn-catalog:/packages/flynn-ide-tools/modules/spawn-catalog.bas:Ring-3 program spawn hints:system\n"
             "\n",
             3943);

    seed_one("/packages/flynn-ide-tools/modules/disc-status.bas",
             "10 REM IDE module: disc-status\n"
             "20 GRID.CLS\n"
             "30 PRINT \"=== Identity Disc ===\"\n"
             "40 PRINT GRID.DISC.STATUS$\n"
             "50 PRINT \"Entity: \"; GRID.DISC.ENTITY$\n"
             "60 PRINT \"Level: \"; GRID.DISC.LEVEL\n"
             "70 PRINT \"XP: \"; GRID.DISC.XP\n"
             "80 END\n"
             "\n",
             215);

    seed_one("/packages/flynn-ide-tools/modules/grid-ping.bas",
             "10 REM IDE module: grid-ping\n"
             "20 PRINT \"=== Grid Ping ===\"\n"
             "30 PRINT \"gateway: \"; GRID.PING(\"gateway\")\n"
             "40 PRINT \"grid: \"; GRID.PING(\"grid\")\n"
             "50 PRINT \"bridge: \"; GRID.PING(\"bridge\")\n"
             "60 END\n"
             "\n",
             186);

    seed_one("/packages/flynn-ide-tools/modules/patrol-arm.bas",
             "10 REM IDE module: patrol-arm\n"
             "20 GRID.RECOGNIZER.START\n"
             "30 PRINT GRID.RECOGNIZER.STATUS$\n"
             "40 END\n"
             "\n",
             95);

    seed_one("/packages/flynn-ide-tools/modules/patrol-stand-down.bas",
             "10 REM IDE module: patrol-stand-down\n"
             "20 GRID.RECOGNIZER.STOP\n"
             "30 PRINT GRID.RECOGNIZER.STATUS$\n"
             "40 END\n"
             "\n",
             101);

    seed_one("/packages/flynn-ide-tools/modules/whoami-panel.bas",
             "10 REM IDE module: whoami-panel\n"
             "20 PRINT \"=== Who Am I ===\"\n"
             "30 PRINT \"Entity: \"; GRID.WHOAMI$\n"
             "40 PRINT \"Disc: \"; GRID.DISC.ENTITY$\n"
             "50 PRINT GRID.STATUS$\n"
             "60 END\n"
             "\n",
             160);

    seed_one("/packages/flynn-ide-tools/modules/caps-panel.bas",
             "10 REM IDE module: caps-panel\n"
             "20 PRINT \"=== Capabilities ===\"\n"
             "30 PRINT \"CAP mask: \"; GRID.CAPS$\n"
             "40 PRINT \"Use 'caps' in shell for decoded list\"\n"
             "50 END\n"
             "\n",
             151);

    seed_one("/packages/flynn-ide-tools/modules/net-status.bas",
             "10 REM IDE module: net-status\n"
             "20 PRINT \"=== Grid Network ===\"\n"
             "30 PRINT GRID.NET.STATUS$\n"
             "40 END\n"
             "\n",
             95);

    seed_one("/packages/flynn-ide-tools/modules/dns-lookup.bas",
             "10 REM IDE module: dns-lookup\n"
             "20 PRINT \"=== DNS Resolve ===\"\n"
             "30 PRINT \"gateway -> \"; GRID.DNS.RESOLVE$(\"gateway\")\n"
             "40 PRINT \"grid -> \"; GRID.DNS.RESOLVE$(\"grid\")\n"
             "50 PRINT \"bridge -> \"; GRID.DNS.RESOLVE$(\"bridge\")\n"
             "60 END\n"
             "\n",
             219);

    seed_one("/packages/flynn-ide-tools/modules/vault-nodes.bas",
             "10 REM IDE module: vault-nodes\n"
             "20 PRINT \"=== Vault Nodes ===\"\n"
             "30 PRINT GRID.VAULT.LIST$\n"
             "40 PRINT \"Put: GRID.VAULT.PUT key$, val$  Sync: GRID.VAULT.SYNC\"\n"
             "50 END\n"
             "\n",
             160);

    seed_one("/packages/flynn-ide-tools/modules/gfs-programs.bas",
             "10 REM IDE module: gfs-programs\n"
             "20 PRINT \"=== Flynn /programs ===\"\n"
             "30 PRINT GRID.GFS.LIST$(\"/programs\")\n"
             "40 PRINT \"Run: basic run /programs/hello.bas\"\n"
             "50 END\n"
             "\n",
             157);

    seed_one("/packages/flynn-ide-tools/modules/jobs-monitor.bas",
             "10 REM IDE module: jobs-monitor\n"
             "20 PRINT \"=== Background Jobs ===\"\n"
             "30 PRINT GRID.JOBS.LIST$\n"
             "40 PRINT \"Shell: jobs   kill <#>   wait\"\n"
             "50 END\n"
             "\n",
             140);

    seed_one("/packages/flynn-ide-tools/modules/iso-roster.bas",
             "10 REM IDE module: iso-roster\n"
             "20 PRINT \"=== ISO Zone ===\"\n"
             "30 PRINT GRID.ISO.LIST$\n"
             "40 PRINT \"Shell: iso list   iso spawn\"\n"
             "50 END\n"
             "\n",
             128);

    seed_one("/packages/flynn-ide-tools/modules/audit-tail.bas",
             "10 REM IDE module: audit-tail\n"
             "20 PRINT \"=== Audit Tail ===\"\n"
             "30 PRINT GRID.LOG.TAIL$(8)\n"
             "40 END\n"
             "\n",
             94);

    seed_one("/packages/flynn-ide-tools/modules/grid-clock.bas",
             "10 REM IDE module: grid-clock\n"
             "20 PRINT \"=== Grid Clock ===\"\n"
             "30 PRINT \"Ticks: \"; GRID.TIME\n"
             "40 FOR I = 1 TO 3\n"
             "50   PRINT \"  beat \"; I; \" @ \"; GRID.TIME\n"
             "60   GRID.WAIT 5\n"
             "70 NEXT I\n"
             "80 END\n"
             "\n",
             184);

    seed_one("/packages/flynn-ide-tools/modules/grid-clear.bas",
             "10 REM IDE module: grid-clear\n"
             "20 GRID.CLS\n"
             "30 PRINT \"=== Flynn GridBASIC IDE ===\"\n"
             "40 PRINT GRID.STATUS$\n"
             "50 PRINT \"Esc :help   :mods   :run\"\n"
             "60 END\n"
             "\n",
             146);

    seed_one("/packages/flynn-ide-tools/modules/pkg-index.bas",
             "10 REM IDE module: pkg-index\n"
             "20 PRINT \"=== Grid Packages ===\"\n"
             "30 PRINT \"Packages: \"; GRID.PKG.LIST$\n"
             "40 PRINT \"Modules: \"; GRID.PKG.MODS$\n"
             "50 PRINT \"Shell: pkg mods   basic mod run <name>\"\n"
             "60 END\n"
             "\n",
             194);

    seed_one("/packages/flynn-ide-tools/modules/sample-menu.bas",
             "10 REM IDE module: sample-menu\n"
             "20 PRINT \"=== GridBASIC Samples ===\"\n"
             "30 PRINT GRID.GFS.LIST$(\"/programs\")\n"
             "40 PRINT \"Try: tutorial, hello, subdemo, grid2d, demo, btc-demo\"\n"
             "50 PRINT \"IDE: Esc :load tutorial   :run demo.grid\"\n"
             "60 END\n"
             "\n",
             230);

    seed_one("/packages/flynn-ide-tools/modules/ide-cheatsheet.bas",
             "10 REM IDE module: ide-cheatsheet\n"
             "20 PRINT \"=== IDE Cheatsheet ===\"\n"
             "30 PRINT \":run :save :load :new :list :find :goto\"\n"
             "40 PRINT \":mods [cat] :mod run <n> :pkg list|mods\"\n"
             "50 PRINT \":tutorial :compile :samples :help\"\n"
             "60 PRINT \"grid> pkg mods network   basic mod run <n>\"\n"
             "70 END\n"
             "\n",
             276);

    seed_one("/packages/flynn-ide-tools/modules/beep-scale.bas",
             "10 REM IDE module: beep-scale\n"
             "20 PRINT \"=== Grid Beep ===\"\n"
             "30 GRID.NOTE 60, 120\n"
             "40 GRID.NOTE 64, 120\n"
             "50 GRID.NOTE 67, 120\n"
             "60 GRID.BEEP 880, 200\n"
             "70 PRINT \"End of line.\"\n"
             "80 END\n"
             "\n",
             175);

    seed_one("/packages/flynn-ide-tools/modules/plot-grid.bas",
             "10 REM IDE module: plot-grid\n"
             "20 GRID.CLS\n"
             "30 FOR X = 0 TO 40\n"
             "40   GRID.PLOT X, X, 2\n"
             "50   GRID.PLOT 80 - X, X, 3\n"
             "60 NEXT X\n"
             "70 PRINT \"Plot demo complete.\"\n"
             "80 END\n"
             "\n",
             159);

    seed_one("/packages/flynn-ide-tools/modules/ai-ask.bas",
             "10 REM IDE module: ai-ask (host: make ai-bridge)\n"
             "20 PRINT \"=== Grid AI ===\"\n"
             "30 PRINT GRID.AI.MODELS$\n"
             "40 PRINT GRID.AI.ASK$(\"What is PRINT in GridBASIC?\", \"EXPLAIN\")\n"
             "50 END\n"
             "\n",
             172);

    seed_one("/packages/flynn-ide-tools/modules/btc-snapshot.bas",
             "10 REM IDE module: btc-snapshot (host: make btc-bridge)\n"
             "20 PRINT \"=== Grid BTC ===\"\n"
             "30 PRINT GRID.BTC.STATUS$\n"
             "40 PRINT GRID.BTC.HELP$\n"
             "50 END\n"
             "\n",
             141);

    seed_one("/packages/flynn-ide-tools/modules/irc-check.bas",
             "10 REM IDE module: irc-check\n"
             "20 PRINT \"=== Grid IRC ===\"\n"
             "30 PRINT GRID.IRC.STATUS$\n"
             "40 PRINT \"Connect: irc connect gateway 6667 griduser\"\n"
             "50 END\n"
             "\n",
             144);

    seed_one("/packages/flynn-ide-tools/modules/hosts-table.bas",
             "10 REM IDE module: hosts-table\n"
             "20 PRINT \"=== /etc/hosts ===\"\n"
             "30 H$ = GRID.GFS.READ$(\"/etc/hosts\")\n"
             "40 IF LEN(H$) > 0 THEN PRINT H$ ELSE PRINT \"(missing — gfs seed)\"\n"
             "50 END\n"
             "\n",
             173);

    seed_one("/packages/flynn-ide-tools/modules/spawn-catalog.bas",
             "10 REM IDE module: spawn-catalog\n"
             "20 PRINT \"=== Spawn Catalog ===\"\n"
             "30 PRINT GRID.GFS.LIST$(\"/programs\")\n"
             "40 PRINT \"Shell: spawn gridsh   spawn lightcycle\"\n"
             "50 PRINT \"GRID.SPAWN.BG runs jobs in background\"\n"
             "60 END\n"
             "\n",
             209);

    seed_one("/packages/flynn-net-tools/MANIFEST",
             "name=flynn-net-tools\n"
             "version=1.0\n"
             "desc=Flynn network bridge helpers for GridBASIC IDE\n"
             "file=/packages/flynn-net-tools/MANIFEST\n"
             "file=/packages/flynn-net-tools/modules/http-probe.bas\n"
             "file=/packages/flynn-net-tools/modules/irc-connect.bas\n"
             "file=/packages/flynn-net-tools/modules/https-bridge.bas\n"
             "mod=http-probe:/packages/flynn-net-tools/modules/http-probe.bas:HTTP GET probe via GRID.HTTP:network\n"
             "mod=irc-connect:/packages/flynn-net-tools/modules/irc-connect.bas:IRC quick-connect helper:network\n"
             "mod=https-bridge:/packages/flynn-net-tools/modules/https-bridge.bas:HTTPS bridge status (host bridge):bridge\n"
             "\n",
             599);

    seed_one("/packages/flynn-net-tools/modules/http-probe.bas",
             "10 REM Flynn net-tools: http-probe\n"
             "20 PRINT \"=== HTTP Probe ===\"\n"
             "30 R$ = GRID.HTTP.GET$(\"gateway\", 80, \"/\")\n"
             "40 IF LEN(R$) > 0 THEN PRINT \"HTTP ok (\"; LEN(R$); \" B)\" ELSE PRINT \"HTTP skip (no bridge)\"\n"
             "50 END\n"
             "\n",
             207);

    seed_one("/packages/flynn-net-tools/modules/irc-connect.bas",
             "10 REM Flynn net-tools: irc-connect\n"
             "20 PRINT \"=== IRC Connect ===\"\n"
             "30 PRINT GRID.IRC.STATUS$\n"
             "40 PRINT \"Try: irc connect gateway 6667 griduser\"\n"
             "50 PRINT \"Then: irc join #grid   irc say #grid hello\"\n"
             "60 END\n"
             "\n",
             204);

    seed_one("/packages/flynn-net-tools/modules/https-bridge.bas",
             "10 REM Flynn net-tools: https-bridge (host: make https-bridge)\n"
             "20 PRINT \"=== HTTPS Bridge ===\"\n"
             "30 PRINT \"Host: make https-bridge\"\n"
             "40 PRINT \"Guest HTTP via GRID.HTTP.* on port 80\"\n"
             "50 PRINT GRID.NET.STATUS$\n"
             "60 END\n"
             "\n",
             212);

    return 0;
}
