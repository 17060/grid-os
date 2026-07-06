#include "console.h"
#include "disk.h"
#include "iso.h"
#include "security.h"
#include "serial.h"
#include "storage.h"

#include <stddef.h>
#include <stdint.h>

#define GRID_VAULT_MAGIC 0x47524431u
#define GRID_VAULT_VERSION 6u
#define VAULT_DISK_LBA 32u
#define VAULT_DISK_SECTORS 3u
#define VAULT_DISK_SIG_LBA 31u

typedef struct {
    int used;
    char key[VAULT_KEY_MAX];
    char value[VAULT_VAL_MAX];
} vault_entry_t;

typedef struct {
    int used;
    char name[ISO_NAME_MAX];
    uint8_t disc[16];
    uint8_t genome[ISO_GENOME_SIZE];
    uint32_t generation;
    uint32_t fitness;
    iso_state_t state;
} vault_iso_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t checksum;
    uint8_t user_disc[GRID_DISC_BYTES];
    uint32_t cycles;
    vault_iso_t isos[ISO_ZONE_SLOTS];
    uint32_t entry_count;
    vault_entry_t entries[VAULT_ENTRIES];
} grid_vault_t;

static grid_vault_t vault;

static int equals_key(const char *a, const char *b) {
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

static uint32_t vault_checksum(const grid_vault_t *data) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    size_t limit = sizeof(grid_vault_t) - sizeof(uint32_t);

    for (size_t i = 0; i < limit; ++i) {
        crc = crc32_update(crc, bytes[i]);
    }
    return crc ^ 0xFFFFFFFFu;
}

static void vault_recompute_checksum(void) {
    vault.checksum = 0;
    vault.checksum = vault_checksum(&vault);
}

static int vault_valid(const grid_vault_t *data) {
    if (data->magic != GRID_VAULT_MAGIC || data->version != GRID_VAULT_VERSION) {
        return 0;
    }
    return data->checksum == vault_checksum(data);
}

static int vault_put_raw(const char *key, const char *value) {
    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        if (vault.entries[i].used && equals_key(vault.entries[i].key, key)) {
            copy_string(vault.entries[i].value, VAULT_VAL_MAX, value);
            return 0;
        }
    }

    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        if (!vault.entries[i].used) {
            vault.entries[i].used = 1;
            copy_string(vault.entries[i].key, VAULT_KEY_MAX, key);
            copy_string(vault.entries[i].value, VAULT_VAL_MAX, value);
            vault.entry_count++;
            return 0;
        }
    }
    return -1;
}

static void capture_iso_state(void) {
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        const iso_entity_t *iso = iso_get(i + 1);
        vault_iso_t *slot = &vault.isos[i];

        if (!iso) {
            slot->used = 0;
            continue;
        }

        slot->used = 1;
        copy_string(slot->name, ISO_NAME_MAX, iso->name);
        for (size_t b = 0; b < 16; ++b) {
            slot->disc[b] = iso->disc[b];
        }
        for (size_t b = 0; b < ISO_GENOME_SIZE; ++b) {
            slot->genome[b] = iso->genome[b];
        }
        slot->generation = iso->generation;
        slot->fitness = iso->fitness;
        slot->state = iso->state;
    }
}

static void restore_iso_state(void) {
    iso_clear_all();
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        const vault_iso_t *slot = &vault.isos[i];
        if (!slot->used) {
            continue;
        }
        iso_import_entity(i + 1, slot->name, slot->disc, slot->genome, slot->generation,
                          slot->fitness, slot->state);
    }
}

static uint8_t hex_nibble(char c) {
    if (c >= '0' && c <= '9') {
        return (uint8_t)(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return (uint8_t)(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return (uint8_t)(c - 'a' + 10);
    }
    return 0;
}

void storage_init(void) {
    disk_init();
    if (storage_load_disk() == 0) {
        storage_restore();
        return;
    }
    if (vault_valid(&vault)) {
        storage_restore();
    }
}

int storage_disk_present(void) {
    return disk_present();
}

static int write_disk_signature(void) {
    uint8_t sector[DISK_SECTOR_SIZE];
    for (size_t i = 0; i < DISK_SECTOR_SIZE; ++i) {
        sector[i] = 0;
    }
    sector[0] = 'F';
    sector[1] = 'L';
    sector[2] = 'Y';
    sector[3] = 'N';
    sector[4] = 'G';
    sector[5] = 'R';
    sector[6] = 'I';
    sector[7] = 'D';
    sector[8] = (uint8_t)GRID_VAULT_VERSION;
    return disk_write(VAULT_DISK_SIG_LBA, sector);
}

static int disk_signature_valid(void) {
    uint8_t sector[DISK_SECTOR_SIZE];
    if (disk_read(VAULT_DISK_SIG_LBA, sector) != 0) {
        return 0;
    }
    return sector[0] == 'F' && sector[1] == 'L' && sector[2] == 'Y' &&
           sector[3] == 'N' && sector[4] == 'G' && sector[5] == 'R' &&
           sector[6] == 'I' && sector[7] == 'D' &&
           sector[8] == (uint8_t)GRID_VAULT_VERSION;
}

int storage_load_disk(void) {
    uint8_t sector[DISK_SECTOR_SIZE];
    uint8_t *raw = (uint8_t *)&vault;

    if (!disk_present() || !disk_signature_valid()) {
        return -1;
    }

    for (uint32_t i = 0; i < VAULT_DISK_SECTORS; ++i) {
        if (disk_read(VAULT_DISK_LBA + i, sector) != 0) {
            return -1;
        }
        for (size_t b = 0; b < DISK_SECTOR_SIZE; ++b) {
            size_t offset = (size_t)i * DISK_SECTOR_SIZE + b;
            if (offset < sizeof(grid_vault_t)) {
                raw[offset] = sector[b];
            }
        }
    }

    if (!vault_valid(&vault)) {
        return -1;
    }
    return 0;
}

int storage_sync_disk(void) {
    uint8_t sector[DISK_SECTOR_SIZE];
    uint8_t *raw = (uint8_t *)&vault;

    if (!disk_present()) {
        return -1;
    }

    storage_snapshot();
    if (write_disk_signature() != 0) {
        return -1;
    }

    for (uint32_t i = 0; i < VAULT_DISK_SECTORS; ++i) {
        for (size_t b = 0; b < DISK_SECTOR_SIZE; ++b) {
            size_t offset = (size_t)i * DISK_SECTOR_SIZE + b;
            if (offset < sizeof(grid_vault_t)) {
                sector[b] = raw[offset];
            } else {
                sector[b] = 0;
            }
        }
        if (disk_write(VAULT_DISK_LBA + i, sector) != 0) {
            return -1;
        }
    }

    return 0;
}

int storage_copy_node(const char *key, char *out, size_t out_len) {
    const char *value = storage_get(key);
    if (!value || out_len == 0) {
        return -1;
    }
    copy_string(out, out_len, value);
    return 0;
}

int storage_is_valid(void) {
    return vault_valid(&vault);
}

int storage_put(const char *key, const char *value) {
    if (!key || !value) {
        return -1;
    }
    if (!security_has_capability(CAP_STORAGE)) {
        return -1;
    }

    if (vault_put_raw(key, value) != 0) {
        return -1;
    }
    vault_recompute_checksum();
    return 0;
}

const char *storage_get(const char *key) {
    if (!key) {
        return 0;
    }

    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        if (vault.entries[i].used && equals_key(vault.entries[i].key, key)) {
            return vault.entries[i].value;
        }
    }

    return 0;
}

void storage_list(void) {
    console_write_line("Grid Vault nodes:");
    int found = 0;

    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        if (!vault.entries[i].used) {
            continue;
        }
        found = 1;
        console_write("  ");
        console_set_color(GRID_COL_OK);
        console_write(vault.entries[i].key);
        console_set_color(GRID_COL_DEFAULT);
        console_write(" = ");
        console_write_line(vault.entries[i].value);
    }

    if (!found) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (empty — use 'vault put <key> <value>')");
        console_set_color(GRID_COL_DEFAULT);
    }
}

int storage_snapshot(void) {
    const grid_identity_t *identity = security_current_identity();

    vault.magic = GRID_VAULT_MAGIC;
    vault.version = GRID_VAULT_VERSION;
    for (size_t i = 0; i < GRID_DISC_BYTES; ++i) {
        vault.user_disc[i] = identity->disc[i];
    }
    vault.cycles = security_cycles();
    capture_iso_state();
    vault_recompute_checksum();
    return 0;
}

int storage_restore(void) {
    if (!vault_valid(&vault)) {
        return -1;
    }

    security_restore_state(vault.user_disc, vault.cycles);
    restore_iso_state();
    return 0;
}

static void write_hex_byte(uint8_t value) {
    static const char hex[] = "0123456789ABCDEF";
    serial_write_byte(hex[(value >> 4) & 0xF]);
    serial_write_byte(hex[value & 0xF]);
}

static void write_hex_buffer(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        write_hex_byte(data[i]);
    }
}

static void write_uint_serial(uint32_t value) {
    char buffer[16];
    size_t pos = 0;

    if (value == 0) {
        serial_write("0");
        return;
    }

    while (value > 0) {
        buffer[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (pos > 0) {
        serial_write_byte(buffer[--pos]);
    }
}

void storage_export_serial(void) {
    storage_snapshot();

    serial_write("#GRIDOS/0.5\n");
    serial_write("DISC=");
    write_hex_buffer(vault.user_disc, GRID_DISC_BYTES);
    serial_write("\nCYCLES=");
    write_uint_serial(vault.cycles);
    serial_write("\n");

    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        if (!vault.isos[i].used) {
            continue;
        }
        serial_write("ISO:");
        serial_write_byte((char)('0' + i + 1));
        serial_write(":");
        serial_write(vault.isos[i].name);
        serial_write(":");
        write_uint_serial(vault.isos[i].generation);
        serial_write(":");
        write_hex_buffer(vault.isos[i].genome, ISO_GENOME_SIZE);
        serial_write("\n");
    }

    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        if (!vault.entries[i].used) {
            continue;
        }
        serial_write("VAULT:");
        serial_write(vault.entries[i].key);
        serial_write("=");
        serial_write(vault.entries[i].value);
        serial_write("\n");
    }

    serial_write("END\n");

    console_set_color(GRID_COL_OK);
    console_write_line("Grid state exported on COM1 serial.");
    console_set_color(GRID_COL_DEFAULT);
}

static int parse_iso_line(const char *line) {
    int id = line[4] - '0';
    if (id < 1 || id > ISO_ZONE_SLOTS) {
        return -1;
    }

    char name[ISO_NAME_MAX];
    uint8_t genome[ISO_GENOME_SIZE];
    uint32_t generation = 0;
    size_t i = 6;

    size_t n = 0;
    while (line[i] && line[i] != ':' && n + 1 < ISO_NAME_MAX) {
        name[n++] = line[i++];
    }
    name[n] = '\0';
    if (line[i] == ':') {
        i++;
    }

    while (line[i] >= '0' && line[i] <= '9') {
        generation = (generation * 10u) + (uint32_t)(line[i] - '0');
        i++;
    }
    if (line[i] == ':') {
        i++;
    }

    for (size_t g = 0; g < ISO_GENOME_SIZE; ++g) {
        genome[g] = (uint8_t)((hex_nibble(line[i]) << 4) | hex_nibble(line[i + 1]));
        i += 2;
    }

    vault.isos[id - 1].used = 1;
    copy_string(vault.isos[id - 1].name, ISO_NAME_MAX, name);
    vault.isos[id - 1].generation = generation;
    for (size_t g = 0; g < ISO_GENOME_SIZE; ++g) {
        vault.isos[id - 1].genome[g] = genome[g];
    }
    vault.isos[id - 1].fitness = generation;
    vault.isos[id - 1].state = ISO_STATE_ACTIVE;
    return 0;
}

int storage_import_serial(void) {
    char line[128];
    int got_header = 0;
    int got_end = 0;

    vault.magic = GRID_VAULT_MAGIC;
    vault.version = GRID_VAULT_VERSION;
    vault.entry_count = 0;
    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        vault.entries[i].used = 0;
    }
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        vault.isos[i].used = 0;
    }

    console_write_line("Waiting for serial import (paste export, finish with END)...");

    for (int attempt = 0; attempt < 512; ++attempt) {
        size_t len = serial_read_line(line, sizeof(line), 5000000);
        if (len == 0) {
            continue;
        }

        if (line[0] == '#') {
            got_header = 1;
            continue;
        }

        if (line[0] == 'D' && line[1] == 'I') {
            for (size_t i = 0; i < GRID_DISC_BYTES; ++i) {
                vault.user_disc[i] =
                    (uint8_t)((hex_nibble(line[5 + (i * 2)]) << 4) | hex_nibble(line[6 + (i * 2)]));
            }
            continue;
        }

        if (line[0] == 'C' && line[1] == 'Y') {
            uint32_t cycles = 0;
            const char *cursor = line + 7;
            while (*cursor >= '0' && *cursor <= '9') {
                cycles = (cycles * 10u) + (uint32_t)(*cursor - '0');
                cursor++;
            }
            vault.cycles = cycles;
            continue;
        }

        if (line[0] == 'I' && line[1] == 'S') {
            parse_iso_line(line);
            continue;
        }

        if (line[0] == 'V' && line[1] == 'A') {
            char key[VAULT_KEY_MAX];
            char value[VAULT_VAL_MAX];
            size_t i = 6;
            size_t k = 0;
            while (line[i] && line[i] != '=' && k + 1 < VAULT_KEY_MAX) {
                key[k++] = line[i++];
            }
            key[k] = '\0';
            if (line[i] == '=') {
                i++;
            }
            copy_string(value, VAULT_VAL_MAX, line + i);
            vault_put_raw(key, value);
            continue;
        }

        if (line[0] == 'E' && line[1] == 'N' && line[2] == 'D') {
            got_end = 1;
            break;
        }
    }

    if (!got_header || !got_end) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("Import failed or timed out.");
        console_set_color(GRID_COL_DEFAULT);
        return -1;
    }

    vault_recompute_checksum();
    storage_restore();

    console_set_color(GRID_COL_OK);
    console_write_line("Grid state imported from serial.");
    console_set_color(GRID_COL_DEFAULT);
    return 0;
}
