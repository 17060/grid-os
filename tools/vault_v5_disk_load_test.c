/* Host test: load v5 vault bytes from a patched disk image file. */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define GRID_VAULT_MAGIC 0x47524431u
#define GRID_VAULT_VERSION 6u
#define GRID_VAULT_VERSION_V5 5u
#define GRID_DISC_BYTES 16
#define ISO_ZONE_SLOTS 4
#define ISO_GENOME_SIZE 32
#define ISO_NAME_MAX 16
#define VAULT_KEY_MAX 16
#define VAULT_VAL_MAX 64
#define VAULT_ENTRIES 8
#define DISK_SECTOR_SIZE 512u
#define VAULT_SIG_LBA 31u
#define VAULT_LBA 32u
#define VAULT_SECTORS_V5 2u

typedef enum {
    ISO_STATE_DORMANT = 0,
    ISO_STATE_ACTIVE = 1,
    ISO_STATE_QUARANTINED = 2
} iso_state_t;

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

int main(int argc, char **argv) {
    FILE *f;
    uint8_t sector[DISK_SECTOR_SIZE];
    grid_vault_t vault;
    uint8_t version = 0;
    size_t n;

    if (argc < 2) {
        fprintf(stderr, "usage: %s build/grid-test.img\n", argv[0]);
        return 1;
    }

    f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    if (fseek(f, (long)VAULT_SIG_LBA * (long)DISK_SECTOR_SIZE, SEEK_SET) != 0 ||
        fread(sector, 1, DISK_SECTOR_SIZE, f) != DISK_SECTOR_SIZE) {
        fprintf(stderr, "read sig failed\n");
        return 1;
    }
    if (sector[0] != 'F' || sector[1] != 'L' || sector[2] != 'Y' || sector[3] != 'N' ||
        sector[4] != 'G' || sector[5] != 'R' || sector[6] != 'I' || sector[7] != 'D') {
        fprintf(stderr, "bad sig magic\n");
        return 1;
    }
    version = sector[8];
    printf("disk sig version=%u\n", version);

    memset(&vault, 0, sizeof(vault));
    for (uint32_t i = 0; i < VAULT_SECTORS_V5; ++i) {
        if (fseek(f, (long)(VAULT_LBA + i) * (long)DISK_SECTOR_SIZE, SEEK_SET) != 0 ||
            fread(sector, 1, DISK_SECTOR_SIZE, f) != DISK_SECTOR_SIZE) {
            fprintf(stderr, "read vault sector failed\n");
            return 1;
        }
        n = (size_t)i * DISK_SECTOR_SIZE;
        memcpy((uint8_t *)&vault + n, sector, DISK_SECTOR_SIZE);
    }

    printf("vault magic=%#x version=%u cycles=%u checksum=%#x struct=%zu\n",
           vault.magic, vault.version, vault.cycles, vault.checksum, sizeof(vault));
    printf("v5 valid=%d\n", vault_valid_version(&vault, GRID_VAULT_VERSION_V5));
    fclose(f);
    return vault_valid_version(&vault, GRID_VAULT_VERSION_V5) ? 0 : 1;
}
