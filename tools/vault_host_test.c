/* Host-side tests for vault checksum, capacity, and v5 migration logic. */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "iso.h"

#define GRID_VAULT_MAGIC 0x47524431u
#define GRID_VAULT_VERSION 6u
#define GRID_VAULT_VERSION_V5 5u
#define GRID_DISC_BYTES 16
#define VAULT_KEY_MAX 16
#define VAULT_VAL_MAX 64
#define VAULT_ENTRIES 8

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

static int vault_valid_version(const grid_vault_t *data, uint32_t version) {
    grid_vault_t tmp;

    if (data->magic != GRID_VAULT_MAGIC || data->version != version) {
        return 0;
    }
    tmp = *data;
    tmp.checksum = 0;
    return data->checksum == vault_checksum(&tmp);
}

static void vault_recompute_checksum(grid_vault_t *vault) {
    vault->checksum = 0;
    vault->checksum = vault_checksum(vault);
}

static int vault_put_raw(grid_vault_t *vault, const char *key, const char *value) {
    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        if (vault->entries[i].used && strcmp(vault->entries[i].key, key) == 0) {
            strncpy(vault->entries[i].value, value, VAULT_VAL_MAX - 1);
            vault->entries[i].value[VAULT_VAL_MAX - 1] = '\0';
            return 0;
        }
    }
    for (int i = 0; i < (int)VAULT_ENTRIES; ++i) {
        if (!vault->entries[i].used) {
            vault->entries[i].used = 1;
            strncpy(vault->entries[i].key, key, VAULT_KEY_MAX - 1);
            vault->entries[i].key[VAULT_KEY_MAX - 1] = '\0';
            strncpy(vault->entries[i].value, value, VAULT_VAL_MAX - 1);
            vault->entries[i].value[VAULT_VAL_MAX - 1] = '\0';
            vault->entry_count++;
            return 0;
        }
    }
    return -1;
}

static int test_v5_migration(void) {
    grid_vault_t vault;
    memset(&vault, 0, sizeof(vault));
    vault.magic = GRID_VAULT_MAGIC;
    vault.version = GRID_VAULT_VERSION_V5;
    vault.cycles = 42;
    vault_put_raw(&vault, "node", "alpha");
    vault_recompute_checksum(&vault);

    if (!vault_valid_version(&vault, GRID_VAULT_VERSION_V5)) {
        return -1;
    }

    vault.version = GRID_VAULT_VERSION;
    vault_recompute_checksum(&vault);
    if (!vault_valid_version(&vault, GRID_VAULT_VERSION)) {
        return -1;
    }
    if (vault.cycles != 42) {
        return -1;
    }
    if (strcmp(vault.entries[0].value, "alpha") != 0) {
        return -1;
    }
    return 0;
}

static int test_vault_full(void) {
    grid_vault_t vault;
    char key[16];
    int i;

    memset(&vault, 0, sizeof(vault));
    vault.magic = GRID_VAULT_MAGIC;
    vault.version = GRID_VAULT_VERSION;

    for (i = 0; i < (int)VAULT_ENTRIES; ++i) {
        snprintf(key, sizeof(key), "k%d", i);
        if (vault_put_raw(&vault, key, "v") != 0) {
            return -1;
        }
    }
    if (vault_put_raw(&vault, "overflow", "x") == 0) {
        return -1;
    }
    return 0;
}

static int test_iso_genome_parse(void) {
    const char *line = "ISO:1:alpha:3:0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
    uint8_t genome[ISO_GENOME_SIZE];
    size_t i = 6;
    size_t g;

    while (line[i] && line[i] != ':') {
        i++;
    }
    if (line[i] == ':') {
        i++;
    }
    while (line[i] >= '0' && line[i] <= '9') {
        i++;
    }
    if (line[i] == ':') {
        i++;
    }

    for (g = 0; g < ISO_GENOME_SIZE; ++g) {
        if (!line[i] || !line[i + 1]) {
            return -1;
        }
        genome[g] = (uint8_t)(((line[i] >= 'A' ? line[i] - 'A' + 10 : line[i] - '0') << 4) |
                              (line[i + 1] >= 'A' ? line[i + 1] - 'A' + 10 : line[i + 1] - '0'));
        i += 2;
    }
    if (genome[31] != 0xEF) {
        return -1;
    }
    return 0;
}

int main(void) {
    if (test_v5_migration() != 0) {
        fprintf(stderr, "vault v5 migration test failed\n");
        return 1;
    }
    if (test_vault_full() != 0) {
        fprintf(stderr, "vault full test failed\n");
        return 1;
    }
    if (test_iso_genome_parse() != 0) {
        fprintf(stderr, "iso genome parse test failed\n");
        return 1;
    }
    printf("vault host tests OK\n");
    return 0;
}
