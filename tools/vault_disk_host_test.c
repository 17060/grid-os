/* Host-side vault-on-disk migration simulation (v5 sig → v6 sync). */

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
#define VAULT_DISK_SECTORS 3u
#define VAULT_DISK_SECTORS_V5 2u
#define DISK_SECTOR_SIZE 512u

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

static uint8_t disk[64 * DISK_SECTOR_SIZE];
static uint8_t sig_sector[DISK_SECTOR_SIZE];

static uint32_t crc32_update(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; ++i) {
        crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
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

static void write_sig(uint8_t version) {
    memset(sig_sector, 0, sizeof(sig_sector));
    sig_sector[0] = 'F';
    sig_sector[1] = 'L';
    sig_sector[2] = 'Y';
    sig_sector[3] = 'N';
    sig_sector[4] = 'G';
    sig_sector[5] = 'R';
    sig_sector[6] = 'I';
    sig_sector[7] = 'D';
    sig_sector[8] = version;
}

static void write_vault_to_disk(const grid_vault_t *vault, uint32_t sectors) {
    const uint8_t *raw = (const uint8_t *)vault;
    for (uint32_t s = 0; s < sectors; ++s) {
        uint8_t *sector = disk + s * DISK_SECTOR_SIZE;
        memset(sector, 0, DISK_SECTOR_SIZE);
        for (size_t b = 0; b < DISK_SECTOR_SIZE; ++b) {
            size_t off = (size_t)s * DISK_SECTOR_SIZE + b;
            if (off < sizeof(grid_vault_t)) {
                sector[b] = raw[off];
            }
        }
    }
}

static void load_vault_from_disk(grid_vault_t *vault, uint32_t sectors) {
    uint8_t *raw = (uint8_t *)vault;
    memset(vault, 0, sizeof(*vault));
    for (uint32_t s = 0; s < sectors; ++s) {
        const uint8_t *sector = disk + s * DISK_SECTOR_SIZE;
        for (size_t b = 0; b < DISK_SECTOR_SIZE; ++b) {
            size_t off = (size_t)s * DISK_SECTOR_SIZE + b;
            if (off < sizeof(grid_vault_t)) {
                raw[off] = sector[b];
            }
        }
    }
    size_t loaded = (size_t)sectors * DISK_SECTOR_SIZE;
    if (loaded < sizeof(grid_vault_t)) {
        memset(raw + loaded, 0, sizeof(grid_vault_t) - loaded);
    }
}

static int test_v5_disk_roundtrip(void) {
    grid_vault_t vault;
    grid_vault_t loaded;

    memset(&vault, 0, sizeof(vault));
    vault.magic = GRID_VAULT_MAGIC;
    vault.version = GRID_VAULT_VERSION_V5;
    vault.cycles = 9001;
    vault.checksum = 0;
    vault.checksum = vault_checksum(&vault);
    if (!vault_valid_version(&vault, GRID_VAULT_VERSION_V5)) {
        return -1;
    }

    write_sig(GRID_VAULT_VERSION_V5);
    write_vault_to_disk(&vault, VAULT_DISK_SECTORS_V5);

    load_vault_from_disk(&loaded, VAULT_DISK_SECTORS_V5);
    if (!vault_valid_version(&loaded, GRID_VAULT_VERSION_V5)) {
        return -1;
    }

    loaded.version = GRID_VAULT_VERSION;
    loaded.checksum = 0;
    loaded.checksum = vault_checksum(&loaded);
    write_sig(GRID_VAULT_VERSION);
    write_vault_to_disk(&loaded, VAULT_DISK_SECTORS);

    load_vault_from_disk(&vault, VAULT_DISK_SECTORS);
    if (!vault_valid_version(&vault, GRID_VAULT_VERSION)) {
        return -1;
    }
    if (vault.cycles != 9001) {
        return -1;
    }
    return 0;
}

int main(void) {
    if (test_v5_disk_roundtrip() != 0) {
        fprintf(stderr, "vault disk roundtrip failed\n");
        return 1;
    }
    printf("vault disk host tests OK\n");
    return 0;
}
