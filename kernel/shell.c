#include "basic.h"
#include "console.h"
#include "disk.h"
#include "grid.h"
#include "gfs.h"
#include "gridfs.h"
#include "ide.h"
#include "iso.h"
#include "irc.h"
#include "link.h"
#include "log.h"
#include "net.h"
#include "program.h"
#include "sched.h"
#include "security.h"
#include "serial.h"
#include "storage.h"
#include "timer.h"

#include <stddef.h>
#include <stdint.h>

#define SHELL_INPUT_MAX 128
#define SHELL_ARGS_MAX  8
#define SHELL_HISTORY_MAX 16

static char shell_history[SHELL_HISTORY_MAX][SHELL_INPUT_MAX];
static int shell_history_count = 0;

static uint8_t shell_theme = GRID_COL_DEFAULT;

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

static size_t trim_length(const char *text) {
    size_t len = 0;
    while (text[len]) {
        len++;
    }
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' || text[len - 1] == '\r')) {
        len--;
    }
    return len;
}

static void sanitize_line(char *line) {
    size_t len = trim_length(line);
    line[len] = '\0';
    for (size_t i = 0; i < len; ++i) {
        if (line[i] == '\r') {
            line[i] = '\0';
            break;
        }
    }
}

static int parse_args(char *line, char *argv[], int max_args) {
    int argc = 0;
    char *cursor = line;

    while (*cursor == ' ') {
        cursor++;
    }

    while (*cursor && argc < max_args) {
        argv[argc++] = cursor;
        while (*cursor && *cursor != ' ') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }
        *cursor++ = '\0';
        while (*cursor == ' ') {
            cursor++;
        }
    }

    return argc;
}

static void print_banner(void) {
    console_set_color(GRID_COL_DEFAULT);
    console_write_line("=\\========== GRID OS 6.0 ============/=");
    console_write_line(" FLYNN'S GRID  |  GridBASIC 6.0  |  CODE THE GRID");
    console_write_line("=/======= BASIC // IDE // END OF LINE =====\\=");
    console_set_color(GRID_COL_DIM);
    console_write_line(" On-disk GridFS. Grid Workbench — GEM desktop + AmigaDOS (ide).");
    console_set_color(GRID_COL_DEFAULT);
    if (gfs_present()) {
        console_set_color(GRID_COL_OK);
        console_write_line(" Flynn's arcade disk: GFS2FLYN mounted at LBA 64.");
        console_set_color(GRID_COL_DEFAULT);
    } else if (storage_disk_present()) {
        console_set_color(GRID_COL_WARN);
        console_write_line(" Disk present but GFS not mounted.");
        console_set_color(GRID_COL_DEFAULT);
    }
    if (storage_disk_present()) {
        console_set_color(GRID_COL_OK);
        console_write_line(" Vault persistence ready (vault sync).");
        console_set_color(GRID_COL_DEFAULT);
    }
    console_write_line("");
}

static void cmd_help(void) {
    console_write_line("Commands:");
    console_write_line("  help              Show this help");
    console_write_line("  disc              Display your identity disc");
    console_write_line("  whoami            Show entity type (User / Program)");
    console_write_line("  caps              Show granted capabilities");
    console_write_line("  status            Grid runtime status");
    console_write_line("  cycles            Show elapsed grid cycles");
    console_write_line("  vision            Flynn's founding principles");
    console_write_line("  clear             Clear the screen");
    console_write_line("  echo <text>       Write text to the grid");
    console_write_line("  spawn [name]      Run ring-3 program (foreground)");
    console_write_line("  spawn bg <name>   Load program as background job");
    console_write_line("  jobs              List background sandbox jobs");
    console_write_line("  kill <#>|all       Stop background job(s)");
    console_write_line("  fg <#>            Run background job in foreground");
    console_write_line("  wait              Wait for background jobs to finish");
    console_write_line("  poweroff          Exit QEMU (isa-debug-exit)");
    console_write_line("  catalog           Spawnable ring-3 programs");
    console_write_line("  programs          Spawned program history");
    console_write_line("  ls [path]         GridFS listing");
    console_write_line("  cat <path>        GridFS read");
    console_write_line("  gfs               On-disk Flynn archive");
    console_write_line("  recognizer        Patrol flyover");
    console_write_line("  theme [flynn|clu] Prompt color theme");
    console_write_line("  ide [file.grid]   Grid Workbench (GEM + AmigaDOS)");
    console_write_line("  log [tail]        Audit trail");
    console_write_line("  portal [export|import|recv]  GridLink serial portal");
    console_write_line("  net [status|ping <ip>]       Grid network (virtio-net)");
    console_write_line("  irc <ip> <port> <nick> <#ch> Join an IRC server (TCP)");
    console_write_line("  basic [ide|run <f>|help]     GridBASIC language + IDE");
    console_write_line("  iso               ISO research zone commands");
    console_write_line("  vault             Persistent grid storage");
    console_write_line("  serial            COM1 serial I/O");
    console_write_line("  about             About Grid OS");
}

static void cmd_serial(int argc, char *argv[]) {
    if (argc == 1 || equals(argv[1], "status")) {
        console_write("COM1: ");
        console_set_color(serial_is_online() ? GRID_COL_OK : GRID_COL_ERROR);
        console_write_line(serial_is_online() ? "online (0x3F8)" : "offline");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    if (equals(argv[1], "write") && argc >= 3) {
        if (!security_require_capability(CAP_COMMUNICATE, "serial write")) {
            return;
        }
        for (int i = 2; i < argc; ++i) {
            if (i > 2) {
                serial_write(" ");
            }
            serial_write(argv[i]);
        }
        serial_write("\n");
        console_write_line("Sent on COM1.");
        return;
    }

    if (equals(argv[1], "read")) {
        char line[128];
        size_t len = serial_read_line(line, sizeof(line), 10000000);
        if (len == 0) {
            console_set_color(GRID_COL_DIM);
            console_write_line("(no serial data)");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        console_write("COM1: ");
        console_write_line(line);
        return;
    }

    console_write_line("Serial commands:");
    console_write_line("  serial status          COM1 status");
    console_write_line("  serial write <text>    Transmit on COM1");
    console_write_line("  serial read            Read one line from COM1");
}

static void cmd_vault(int argc, char *argv[]) {
    if (!security_require_capability(CAP_STORAGE, "vault")) {
        return;
    }

    if (argc == 1 || equals(argv[1], "list")) {
        storage_list();
        return;
    }

    if (equals(argv[1], "put") && argc >= 4) {
        if (storage_put(argv[2], argv[3]) != 0) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Vault full or write denied.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        console_set_color(GRID_COL_OK);
        console_write_line("Vault node stored.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    if (equals(argv[1], "get") && argc >= 3) {
        const char *value = storage_get(argv[2]);
        if (!value) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Node not found.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        console_write(argv[2]);
        console_write(" = ");
        console_write_line(value);
        return;
    }

    if (equals(argv[1], "save")) {
        storage_snapshot();
        console_set_color(GRID_COL_OK);
        console_write_line("Grid vault snapshot saved (CRC-sealed, in-memory).");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    if (equals(argv[1], "export")) {
        storage_export_serial();
        return;
    }

    if (equals(argv[1], "import")) {
        storage_import_serial();
        return;
    }

    if (equals(argv[1], "sync")) {
        if (storage_sync_disk() != 0) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Disk sync failed — is Flynn's disk attached?");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        console_set_color(GRID_COL_OK);
        console_write_line("Vault synced to arcade disk (FLYNGRID sectors).");
        console_set_color(GRID_COL_DEFAULT);
        log_event("vault synced to disk");
        return;
    }

    console_write_line("Vault commands:");
    console_write_line("  vault list                 List nodes");
    console_write_line("  vault put <key> <value>    Store node");
    console_write_line("  vault get <key>            Read node");
    console_write_line("  vault save                 Snapshot in memory");
    console_write_line("  vault sync                 Persist to arcade disk");
    console_write_line("  vault export               Export state over COM1");
    console_write_line("  vault import               Import state from COM1");
}

static int parse_iso_id(const char *text, int *out_id) {
    int value = 0;

    if (*text == '\0') {
        return 0;
    }

    while (*text) {
        if (*text < '0' || *text > '9') {
            return 0;
        }
        value = (value * 10) + (*text - '0');
        text++;
    }

    if (value < 1 || value > ISO_ZONE_SLOTS) {
        return 0;
    }

    *out_id = value;
    return 1;
}

static void cmd_iso(int argc, char *argv[]) {
    if (!security_require_capability(CAP_ISO_RESEARCH, "iso")) {
        return;
    }

    if (argc == 1 || equals(argv[1], "zone")) {
        iso_print_zone();
        return;
    }

    if (equals(argv[1], "list")) {
        iso_print_list();
        return;
    }

    if (equals(argv[1], "spawn")) {
        const char *name = (argc >= 3) ? argv[2] : 0;
        int id = iso_spawn(name);
        if (id < 0) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("ISO zone full. Quarantine or wait for a slot.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }

        console_set_color(GRID_COL_OK);
        console_write("ISO spawned in sandbox slot #");
        char idbuf[4];
        idbuf[0] = (char)('0' + id);
        idbuf[1] = '\0';
        console_write_line(idbuf);
        console_set_color(GRID_COL_DIM);
        console_write_line("Emergence welcome. Mutation confined to ISO genome buffer.");
        console_set_color(GRID_COL_DEFAULT);
        iso_print_inspect(id);
        return;
    }

    if (equals(argv[1], "inspect") && argc >= 3) {
        int id = 0;
        if (!parse_iso_id(argv[2], &id)) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Usage: iso inspect <id>");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        iso_print_inspect(id);
        return;
    }

    if (equals(argv[1], "evolve") && argc >= 3) {
        int id = 0;
        if (!parse_iso_id(argv[2], &id)) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Usage: iso evolve <id>");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }

        int result = iso_evolve(id);
        if (result == -2) {
            console_set_color(GRID_COL_WARN);
            console_write_line("ISO is quarantined — observe, do not force evolution.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        if (result != 0) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Evolution rejected by sandbox integrity check.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }

        console_set_color(GRID_COL_OK);
        console_write_line("ISO evolved inside sandbox.");
        console_set_color(GRID_COL_DEFAULT);
        iso_print_inspect(id);
        return;
    }

    if (equals(argv[1], "autopilot") && argc >= 3) {
        if (equals(argv[2], "on")) {
            timer_set_autopilot(1);
            log_event("ISO autopilot enabled");
            console_set_color(GRID_COL_OK);
            console_write_line("ISO autopilot engaged — ISOs evolve in the background.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        if (equals(argv[2], "off")) {
            timer_set_autopilot(0);
            log_event("ISO autopilot disabled");
            console_write_line("ISO autopilot disengaged.");
            return;
        }
    }

    if (equals(argv[1], "quarantine") && argc >= 3) {
        int id = 0;
        if (!parse_iso_id(argv[2], &id)) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Usage: iso quarantine <id>");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }

        if (iso_quarantine(id) != 0) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("ISO not found.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }

        console_set_color(GRID_COL_WARN);
        console_write_line("ISO quarantined (Flynn policy: preserve, do not derez).");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    if (equals(argv[1], "release") && argc >= 3) {
        int id = 0;
        if (!parse_iso_id(argv[2], &id)) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Usage: iso release <id>");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }

        if (iso_release(id) != 0) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("ISO not found.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }

        console_set_color(GRID_COL_OK);
        console_write_line("ISO released back to active research status.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    console_write_line("ISO commands:");
    console_write_line("  iso zone                 Zone status");
    console_write_line("  iso list                 List ISO entities");
    console_write_line("  iso spawn [name]         Seed a new ISO");
    console_write_line("  iso inspect <id>         Inspect genome and disc");
    console_write_line("  iso evolve <id>          Mutate genome in sandbox");
    console_write_line("  iso quarantine <id>      Isolate anomaly (no derez)");
    console_write_line("  iso autopilot on|off     Background evolution");
    console_write_line("  iso release <id>         Restore quarantined ISO");
}

static void cmd_log(int argc, char *argv[]) {
    if (argc >= 2 && equals(argv[1], "tail")) {
        log_print_tail(10);
        return;
    }
    log_print_all();
}

static void cmd_ls(int argc, char *argv[]) {
    const char *path = (argc >= 2) ? argv[1] : "/";
    gridfs_list(path);
}

static void cmd_cat(int argc, char *argv[]) {
    char buffer[512];

    if (argc < 2) {
        console_write_line("Usage: cat <path>");
        return;
    }

    if (gridfs_read(argv[1], buffer, sizeof(buffer)) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("GridFS: path not found.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    console_write_line(buffer);
}

static void cmd_gfs(int argc, char *argv[]) {
    if (argc >= 2 && equals(argv[1], "list")) {
        gfs_list("/");
        return;
    }
    if (argc >= 2 && equals(argv[1], "seed")) {
        if (!security_require_capability(CAP_STORAGE, "gfs seed")) {
            return;
        }
        if (gfs_seed_defaults() == 0) {
            console_set_color(GRID_COL_OK);
            console_write_line("Flynn archive re-seeded on disk.");
            console_set_color(GRID_COL_DEFAULT);
        } else {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Seed failed — is the arcade disk attached?");
            console_set_color(GRID_COL_DEFAULT);
        }
        return;
    }
    gfs_print_status();
}

static void cmd_recognizer(void) {
    console_set_color(GRID_COL_OK);
    console_write_line("        /\\");
    console_write_line("       /  \\___");
    console_write_line("      | REC  |>>  Recognizer patrol");
    console_write_line("       \\__/\\/");
    console_set_color(GRID_COL_DEFAULT);
    console_write_line("Scanning sector... no CLU signatures detected.");
    console_write_line("End of line.");
}

static void cmd_theme(int argc, char *argv[]) {
    if (argc < 2 || equals(argv[1], "flynn")) {
        shell_theme = GRID_COL_OK;
        console_set_color(GRID_COL_OK);
        console_write_line("Theme: Flynn cyan — the Grid is open.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }
    if (equals(argv[1], "clu")) {
        shell_theme = GRID_COL_ERROR;
        console_set_color(GRID_COL_ERROR);
        console_write_line("Theme: CLU corruption — perfection rejected.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }
    console_write_line("Usage: theme flynn | theme clu");
}

static void cmd_ide(int argc, char *argv[]) {
    const char *path = (argc >= 2) ? argv[1] : 0;
    ide_run(path);
    console_clear();
    print_banner();
}

static void cmd_disc(void) {
    char disc[GRID_DISC_HEX];
    security_format_disc(disc, sizeof(disc));
    console_write("Identity Disc: ");
    console_set_color(GRID_COL_OK);
    console_write_line(disc);
    console_set_color(GRID_COL_DEFAULT);
}

static void cmd_whoami(void) {
    console_write("Entity: ");
    console_set_color(GRID_COL_OK);
    console_write_line(security_entity_name());
    console_set_color(GRID_COL_DEFAULT);
}

static void cmd_caps(void) {
    console_write_line("Capabilities:");
    console_write_line(security_has_capability(CAP_READ_GRID) ? "  [x] READ_GRID" : "  [ ] READ_GRID");
    console_write_line(security_has_capability(CAP_WRITE_GRID) ? "  [x] WRITE_GRID" : "  [ ] WRITE_GRID");
    console_write_line(security_has_capability(CAP_SPAWN) ? "  [x] SPAWN" : "  [ ] SPAWN");
    console_write_line(security_has_capability(CAP_COMMUNICATE) ? "  [x] COMMUNICATE" : "  [ ] COMMUNICATE");
    console_write_line(security_has_capability(CAP_ISO_RESEARCH) ? "  [x] ISO_RESEARCH" : "  [ ] ISO_RESEARCH");
    console_write_line(security_has_capability(CAP_STORAGE) ? "  [x] STORAGE" : "  [ ] STORAGE");
    console_write_line(security_has_capability(CAP_ADMIN) ? "  [x] ADMIN (disabled by design)" : "  [ ] ADMIN (disabled by design)");
}

static void cmd_status(void) {
    char buffer[16];

    console_write_line("Grid Status:");
    console_write("  Kernel:        ");
    console_set_color(GRID_COL_OK);
    console_write_line("online");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Security:      ");
    console_set_color(GRID_COL_OK);
    console_write_line("fail-closed, capability-based");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  CLU Protocol:  ");
    console_set_color(GRID_COL_OK);
    console_write_line("absent");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  ISO Zone:      ");
    console_set_color(GRID_COL_OK);
    console_write_line("online");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  ISO Count:     ");
    {
        uint32_t count = (uint32_t)iso_zone_count();
        size_t pos = 0;
        if (count == 0) {
            buffer[pos++] = '0';
        } else {
            char tmp[16];
            size_t tmp_len = 0;
            while (count > 0) {
                tmp[tmp_len++] = (char)('0' + (count % 10));
                count /= 10;
            }
            while (tmp_len > 0) {
                buffer[pos++] = tmp[--tmp_len];
            }
        }
        buffer[pos] = '\0';
    }
    console_write_line(buffer);
    console_write("  Serial:        ");
    console_set_color(serial_is_online() ? GRID_COL_OK : GRID_COL_ERROR);
    console_write_line(serial_is_online() ? "COM1 online" : "offline");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Vault:         ");
    console_set_color(storage_is_valid() ? GRID_COL_OK : GRID_COL_DIM);
    console_write_line(storage_is_valid() ? "snapshot valid" : "empty");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  GridFS:        ");
    console_set_color(gfs_present() ? GRID_COL_OK : GRID_COL_DIM);
    console_write_line(gfs_present() ? "GFS2FLYN on disk" : "not mounted");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Arcade Disk:   ");
    console_set_color(storage_disk_present() ? GRID_COL_OK : GRID_COL_WARN);
    console_write_line(storage_disk_present() ? "FLYNGRID linked" : "not attached");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Disk Driver:   ");
    console_set_color(disk_present() ? GRID_COL_OK : GRID_COL_WARN);
    console_write_line(disk_backend_name());
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Input:         ");
    console_set_color(GRID_COL_OK);
    console_write("PS/2 keyboard");
    console_write_line(" + shift, PS/2 mouse");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Network:       ");
    console_set_color(net_present() ? GRID_COL_OK : GRID_COL_DIM);
    console_write_line(net_present() ? "virtio-net online (10.0.2.15)" : "offline");
    console_set_color(GRID_COL_DEFAULT);
    console_write("  Timer Ticks:   ");
    {
        uint32_t ticks = timer_ticks();
        size_t pos = 0;
        if (ticks == 0) {
            buffer[pos++] = '0';
        } else {
            char tmp[16];
            size_t tmp_len = 0;
            while (ticks > 0) {
                tmp[tmp_len++] = (char)('0' + (ticks % 10));
                ticks /= 10;
            }
            while (tmp_len > 0) {
                buffer[pos++] = tmp[--tmp_len];
            }
        }
        buffer[pos] = '\0';
    }
    console_write_line(buffer);
    console_write("  Autopilot:     ");
    console_write_line(timer_autopilot_enabled() ? "engaged" : "off");
    console_write("  Entity:        ");
    console_write_line(security_entity_name());
    console_write("  Cycles:        ");
    {
        uint32_t cycles = security_cycles();
        size_t pos = 0;
        if (cycles == 0) {
            buffer[pos++] = '0';
        } else {
            char tmp[16];
            size_t tmp_len = 0;
            while (cycles > 0) {
                tmp[tmp_len++] = (char)('0' + (cycles % 10));
                cycles /= 10;
            }
            while (tmp_len > 0) {
                buffer[pos++] = tmp[--tmp_len];
            }
        }
        buffer[pos] = '\0';
    }
    console_write_line(buffer);
}

static void cmd_cycles(void) {
    cmd_status();
}

static void cmd_vision(void) {
    console_set_color(GRID_COL_WARN);
    console_write_line("Flynn's Grid Principles:");
    console_set_color(GRID_COL_DEFAULT);
    console_write_line("  1. The system exists to enable discovery, not enforce perfection.");
    console_write_line("  2. Users retain creative authority; programs run with least privilege.");
    console_write_line("  3. Identity is explicit — every entity carries a disc, not a shared root key.");
    console_write_line("  4. Security fails closed: unknown actions are denied by default.");
    console_write_line("  5. Emergence is a feature. ISOs welcome. Anomalies are not enemies.");
}

static void shell_push_history(const char *line) {
    size_t len = trim_length(line);
    size_t i;

    if (len == 0) {
        return;
    }

    if (shell_history_count > 0 && equals(line, shell_history[shell_history_count - 1])) {
        return;
    }

    if (shell_history_count >= SHELL_HISTORY_MAX) {
        for (i = 1; i < (size_t)SHELL_HISTORY_MAX; ++i) {
            size_t j = 0;
            while (shell_history[i][j] && j + 1 < SHELL_INPUT_MAX) {
                shell_history[i - 1][j] = shell_history[i][j];
                j++;
            }
            shell_history[i - 1][j] = '\0';
        }
        shell_history_count = SHELL_HISTORY_MAX - 1;
    }

    i = 0;
    while (line[i] && i + 1 < SHELL_INPUT_MAX) {
        shell_history[shell_history_count][i] = line[i];
        i++;
    }
    shell_history[shell_history_count][i] = '\0';
    shell_history_count++;
}

static void cmd_net(int argc, char *argv[]) {
    if (argc == 1 || equals(argv[1], "status")) {
        net_print_status();
        return;
    }

    if (equals(argv[1], "ping") && argc >= 3) {
        uint32_t ip;
        if (net_parse_ip(argv[2], &ip) != 0) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Usage: net ping <ip>  (e.g. 10.0.2.2)");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        if (!net_present()) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("No network device. Boot with: make run (virtio-net-pci).");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        console_write("Pinging ");
        console_write(argv[2]);
        console_write_line(" ...");
        int rc = net_ping(ip);
        if (rc == 0) {
            console_set_color(GRID_COL_OK);
            console_write_line("Reply received — the Grid answered.");
            console_set_color(GRID_COL_DEFAULT);
        } else {
            console_set_color(GRID_COL_WARN);
            console_write_line("No reply (timeout). The Grid is silent.");
            console_set_color(GRID_COL_DEFAULT);
        }
        return;
    }

    if (equals(argv[1], "poll")) {
        net_poll();
        console_write_line("Polled RX queue.");
        return;
    }

    console_write_line("Network commands:");
    console_write_line("  net status        Show virtio-net + IP + packet counts");
    console_write_line("  net ping <ip>     Send ICMP echo (e.g. 10.0.2.2)");
    console_write_line("  net poll          Drain the receive queue");
}

static void cmd_irc(int argc, char *argv[]) {
    if (argc < 5) {
        console_write_line("Usage: irc <server-ip> <port> <nick> <#channel>");
        console_write_line("Example: irc 10.0.2.2 6667 gridtest #gridos");
        return;
    }

    uint32_t port = 0;
    for (size_t i = 0; argv[2][i]; ++i) {
        if (argv[2][i] < '0' || argv[2][i] > '9') {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Bad port.");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        port = port * 10u + (uint32_t)(argv[2][i] - '0');
    }
    if (port == 0 || port > 65535u) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("Bad port.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    irc_session(argv[1], (uint16_t)port, argv[3], argv[4], 60000u);
}

static void cmd_basic(int argc, char *argv[]) {
    if (argc < 2) {
        basic_ide(0);
        return;
    }
    if (equals(argv[1], "ide")) {
        basic_ide(argc > 2 ? argv[2] : 0);
        return;
    }
    if (equals(argv[1], "run")) {
        if (argc < 3) {
            console_set_color(GRID_COL_ERROR);
            console_write_line("Usage: basic run <file>");
            console_set_color(GRID_COL_DEFAULT);
            return;
        }
        basic_run_file(argv[2]);
        return;
    }
    if (equals(argv[1], "help") || equals(argv[1], "?")) {
        basic_print_version();
        return;
    }
    console_set_color(GRID_COL_ERROR);
    console_write_line("Usage: basic [ide [file] | run <file> | help]");
    console_set_color(GRID_COL_DEFAULT);
}

static void cmd_basictest(void) {
    /* Deterministic interpreter self-test (no file input required).
     * Uses computed output that cannot appear in the source text, so we can
     * verify execution even through the jumbled serial mirror. */
    static const char *prog =
        "10 LET N = 0\n"
        "20 FOR I = 1 TO 5\n"
        "30 LET N = N + I\n"
        "40 NEXT I\n"
        "50 PRINT \"SUM=\"; N\n"
        "60 LET S$ = \"GRID\"\n"
        "70 PRINT S$; \"-OS\"\n"
        "80 IF N = 15 THEN PRINT \"OK15\" ELSE PRINT \"NO\"\n"
        "90 DIM A(3)\n"
        "100 LET A(0) = 7\n"
        "110 PRINT \"A0=\"; A(0)\n"
        "120 PRINT \"HALF=\"; 7 / 2\n"
        "130 PRINT \"PI=\"; PI\n"
        "140 PRINT \"SQ=\"; SQR(2)\n"
        "150 END\n";
    console_set_color(GRID_COL_TITLE);
    console_write_line("=== GridBASIC self-test ===");
    console_set_color(GRID_COL_DEFAULT);
    basic_run_source(prog);
    console_set_color(GRID_COL_OK);
    console_write_line("[BST-END]");
    console_set_color(GRID_COL_DEFAULT);
}

static void cmd_about(void) {
    console_write_line("Grid OS 6.0 — Flynn's real digital frontier.");
    console_write_line("GridBASIC + IDE · TCP/IRC · ARP/ICMP · true preemptive · GFS2FLYN");
    console_write_line("virtio-blk · serial shell · bg jobs · Ctrl+C · GEM Workbench");
}

static void cmd_portal(int argc, char *argv[]) {
    if (argc >= 2 && equals(argv[1], "export")) {
        if (!security_require_capability(CAP_COMMUNICATE, "portal export")) {
            return;
        }
        gridlink_export_vault();
        return;
    }

    if (argc >= 2 && equals(argv[1], "import")) {
        if (!security_require_capability(CAP_STORAGE, "portal import")) {
            return;
        }
        (void)gridlink_import_vault();
        return;
    }

    if (argc >= 2 && equals(argv[1], "recv")) {
        (void)gridlink_recv_file();
        return;
    }

    console_set_color(GRID_COL_WARN);
    console_write_line("=== GRIDLINK PORTAL ===");
    console_set_color(GRID_COL_DEFAULT);
    console_write_line("Flynn's arcade server is running. The Grid is open.");
    console_write("Disk:   ");
    console_set_color(storage_disk_present() ? GRID_COL_OK : GRID_COL_ERROR);
    console_write_line(storage_disk_present() ? "arcade disk online" : "no disk — run: make run");
    console_set_color(GRID_COL_DEFAULT);
    console_write("Vault:  ");
    console_write_line(storage_is_valid() ? "state sealed" : "fresh frontier");
    console_write("GFS:    ");
    console_write_line(gfs_present() ? "GFS2FLYN mounted" : "not mounted");
    console_set_color(GRID_COL_DIM);
    console_write_line("  portal export     GridLink vault frame on COM1");
    console_write_line("  portal import     receive vault from host");
    console_write_line("  portal recv       install /programs/* over GridLink");
    console_write_line("Host: gridctl install / gridctl portal-push");
    console_set_color(GRID_COL_DEFAULT);
}

static void cmd_catalog(void) {
    program_print_catalog();
}

static void cmd_echo(int argc, char *argv[]) {
    if (argc < 2) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("Usage: echo <text>");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    if (!security_require_capability(CAP_WRITE_GRID, "echo")) {
        return;
    }

    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            console_write(" ");
        }
        console_write(argv[i]);
    }
    console_write_char('\n');
}

static void cmd_spawn(int argc, char *argv[]) {
    const char *name = "gridsh";

    if (!security_require_capability(CAP_SPAWN, "spawn")) {
        return;
    }

    if (argc >= 2) {
        if (equals(argv[1], "list") || equals(argv[1], "catalog")) {
            program_print_catalog();
            return;
        }
        if (equals(argv[1], "bg")) {
            if (argc < 3) {
                console_write_line("Usage: spawn bg <name>");
                return;
            }

            int id = program_run_background(argv[2]);
            if (id < 0) {
                console_set_color(GRID_COL_ERROR);
                console_write_line("Background spawn failed. Try: catalog");
                console_set_color(GRID_COL_DEFAULT);
                return;
            }

            console_set_color(GRID_COL_OK);
            console_write("Background job queued in slot #");
            console_write_char((char)('0' + id));
            console_write_line("");
            console_set_color(GRID_COL_DEFAULT);
            log_event("background program queued");
            return;
        }
        name = argv[1];
    }

    console_set_color(GRID_COL_WARN);
    console_write_line("Entering ring-3 sandbox (W^X enforced)...");
    console_set_color(GRID_COL_DEFAULT);

    int id = program_spawn_named(name);
    if (id < 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("Unknown program or spawn failed. Try: catalog");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    log_event("program spawned");

    console_set_color(GRID_COL_OK);
    console_write("Program returned from sandbox slot #");
    console_write_char((char)('0' + id));
    console_write_line("");
    console_set_color(GRID_COL_DEFAULT);
}

static void cmd_programs(void) {
    program_print_list();
}

static void cmd_jobs(void) {
    sched_print_jobs();
}

static int parse_job_id(const char *text) {
    int id = 0;

    if (!text || *text < '0' || *text > '9') {
        return -1;
    }

    while (*text >= '0' && *text <= '9') {
        id = id * 10 + (*text - '0');
        text++;
    }

    if (*text != '\0' || id <= 0 || id > PROGRAM_SLOTS) {
        return -1;
    }

    return id;
}

static void cmd_kill(int argc, char *argv[]) {
    if (argc < 2) {
        console_write_line("Usage: kill <job#> | kill all");
        return;
    }

    if (equals(argv[1], "all")) {
        sched_kill_all();
        console_set_color(GRID_COL_OK);
        console_write_line("Background jobs cleared.");
        console_set_color(GRID_COL_DEFAULT);
        log_event("background jobs killed");
        return;
    }

    int id = parse_job_id(argv[1]);
    if (id < 0 || sched_kill(id) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("No such background job. Try: jobs");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    console_set_color(GRID_COL_OK);
    console_write("Job #");
    console_write_char((char)('0' + id));
    console_write_line(" stopped.");
    console_set_color(GRID_COL_DEFAULT);
    log_event("background job killed");
}

static void cmd_poweroff(void) {
    console_set_color(GRID_COL_WARN);
    console_write_line("Derezzing Grid OS — returning to the real world.");
    console_set_color(GRID_COL_DEFAULT);
    log_event("poweroff requested");

    /* isa-debug-exit (Makefile: iobase=0xf4) — clean QEMU exit under run-headless */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x01), "Nd"((uint16_t)0xf4));
    /* Legacy Bochs shutdown port */
    __asm__ volatile("outw %0, %1" : : "a"((uint16_t)0), "Nd"((uint16_t)0x501));

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

static void cmd_fg(int argc, char *argv[]) {
    if (argc < 2) {
        console_write_line("Usage: fg <job#>");
        return;
    }

    int id = parse_job_id(argv[1]);
    const grid_program_t *program = program_get(id);
    if (id < 0 || !program) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("No such program slot. Try: jobs or programs");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    if (program->state == PROGRAM_EXITED || program->state == PROGRAM_FAULT) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("Program already ended.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    (void)sched_detach(id);

    console_set_color(GRID_COL_WARN);
    console_write("Foreground job #");
    console_write_char((char)('0' + id));
    console_write_line(" — press keys in program window (WASD for lightcycle).");
    console_set_color(GRID_COL_DEFAULT);

    if (program_run(id) != 0) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("Program faulted in sandbox.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    console_set_color(GRID_COL_OK);
    console_write("Job #");
    console_write_char((char)('0' + id));
    console_write_line(" returned.");
    console_set_color(GRID_COL_DEFAULT);
}

static void cmd_wait(void) {
    if (sched_job_count() == 0) {
        console_set_color(GRID_COL_DIM);
        console_write_line("No background jobs running.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    console_write_line("Waiting for background jobs...");
    while (sched_job_count() > 0) {
        sched_service();
    }

    console_set_color(GRID_COL_OK);
    console_write_line("All background jobs finished.");
    console_set_color(GRID_COL_DEFAULT);
}

static void dispatch_command(char *line) {
    char *argv[SHELL_ARGS_MAX];
    int argc = parse_args(line, argv, SHELL_ARGS_MAX);

    if (argc == 0) {
        return;
    }

    if (equals(argv[0], "help")) {
        cmd_help();
    } else if (equals(argv[0], "disc")) {
        cmd_disc();
    } else if (equals(argv[0], "whoami")) {
        cmd_whoami();
    } else if (equals(argv[0], "caps")) {
        cmd_caps();
    } else if (equals(argv[0], "status") || equals(argv[0], "cycles")) {
        cmd_cycles();
    } else if (equals(argv[0], "vision")) {
        cmd_vision();
    } else if (equals(argv[0], "clear")) {
        console_clear();
        print_banner();
    } else if (equals(argv[0], "about")) {
        cmd_about();
    } else if (equals(argv[0], "echo")) {
        cmd_echo(argc, argv);
    } else if (equals(argv[0], "spawn")) {
        cmd_spawn(argc, argv);
    } else if (equals(argv[0], "catalog")) {
        cmd_catalog();
    } else if (equals(argv[0], "portal")) {
        cmd_portal(argc, argv);
    } else if (equals(argv[0], "net")) {
        cmd_net(argc, argv);
    } else if (equals(argv[0], "irc")) {
        cmd_irc(argc, argv);
    } else if (equals(argv[0], "basic")) {
        cmd_basic(argc, argv);
    } else if (equals(argv[0], "basictest")) {
        cmd_basictest();
    } else if (equals(argv[0], "log")) {
        cmd_log(argc, argv);
    } else if (equals(argv[0], "ls")) {
        cmd_ls(argc, argv);
    } else if (equals(argv[0], "cat")) {
        cmd_cat(argc, argv);
    } else if (equals(argv[0], "programs")) {
        cmd_programs();
    } else if (equals(argv[0], "jobs")) {
        cmd_jobs();
    } else if (equals(argv[0], "kill")) {
        cmd_kill(argc, argv);
    } else if (equals(argv[0], "fg")) {
        cmd_fg(argc, argv);
    } else if (equals(argv[0], "wait")) {
        cmd_wait();
    } else if (equals(argv[0], "poweroff") || equals(argv[0], "halt")) {
        cmd_poweroff();
    } else if (equals(argv[0], "iso")) {
        cmd_iso(argc, argv);
    } else if (equals(argv[0], "vault")) {
        cmd_vault(argc, argv);
    } else if (equals(argv[0], "serial")) {
        cmd_serial(argc, argv);
    } else if (equals(argv[0], "gfs")) {
        cmd_gfs(argc, argv);
    } else if (equals(argv[0], "recognizer")) {
        cmd_recognizer();
    } else if (equals(argv[0], "theme")) {
        cmd_theme(argc, argv);
    } else if (equals(argv[0], "ide") || equals(argv[0], "workshop")) {
        cmd_ide(argc, argv);
    } else {
        console_set_color(GRID_COL_ERROR);
        console_write("Unknown command: ");
        console_write_line(argv[0]);
        console_set_color(GRID_COL_DEFAULT);
        console_write_line("Type 'help' for available commands.");
    }
}

void shell_run(void) {
    char line[SHELL_INPUT_MAX];

    console_set_serial_mirror(1);
    print_banner();
    cmd_disc();
    console_write_line("");
    console_write_line("Type 'help' to explore the frontier.");
    console_write_line("Type 'ide' for Grid Workbench — GEM desktop and AmigaDOS CLI.");
    console_write_line("");

    for (;;) {
        console_set_color(shell_theme);
        console_write("grid> ");
        console_set_color(GRID_COL_DEFAULT);
        console_read_line_hist(line, sizeof(line), shell_history, SHELL_HISTORY_MAX,
                               &shell_history_count);

        sanitize_line(line);
        if (trim_length(line) == 0) {
            continue;
        }

        shell_push_history(line);

        dispatch_command(line);
    }
}
