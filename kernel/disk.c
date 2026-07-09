#include "disk.h"
#include "virtio_blk.h"

#include <stdint.h>

#define ATA_DATA         0x01F0
#define ATA_ERROR        0x01F1
#define ATA_SECTOR_COUNT 0x01F2
#define ATA_LBA_LOW      0x01F3
#define ATA_LBA_MID      0x01F4
#define ATA_LBA_HIGH     0x01F5
#define ATA_LBA_LLD      0x01F6
#define ATA_STATUS       0x01F7
#define ATA_CMD          0x01F7

#define ATA_SR_BSY  0x80
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

typedef enum {
    DISK_BACKEND_NONE = 0,
    DISK_BACKEND_VIRTIO = 1,
    DISK_BACKEND_IDE = 2
} disk_backend_t;

static disk_backend_t backend = DISK_BACKEND_NONE;
static int ide_present = 0;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int ata_wait_busy(void) {
    for (int i = 0; i < 100000; ++i) {
        uint8_t status = inb(ATA_STATUS);
        if ((status & ATA_SR_BSY) == 0) {
            return 0;
        }
    }
    return -1;
}

static int ata_wait_drq(void) {
    for (int i = 0; i < 100000; ++i) {
        uint8_t status = inb(ATA_STATUS);
        if (status & ATA_SR_ERR) {
            return -1;
        }
        if ((status & ATA_SR_BSY) == 0 && (status & ATA_SR_DRQ)) {
            return 0;
        }
    }
    return -1;
}

static void ide_probe(void) {
    ide_present = 0;
    if (ata_wait_busy() != 0) {
        return;
    }

    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_LBA_LLD, 0xE0);
    outb(ATA_CMD, 0xEC);

    if (ata_wait_busy() != 0) {
        return;
    }

    uint8_t mid = inb(ATA_LBA_MID);
    uint8_t high = inb(ATA_LBA_HIGH);
    if (mid == 0 && high == 0) {
        ide_present = 1;
    }
}

static int ide_transfer(uint32_t lba, void *buffer, int write) {
    if (!ide_present || !buffer) {
        return -1;
    }

    if (ata_wait_busy() != 0) {
        return -1;
    }

    outb(ATA_LBA_LLD, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_CMD, (uint8_t)(write ? 0x30 : 0x20));

    if (ata_wait_drq() != 0) {
        return -1;
    }

    uint16_t *words = (uint16_t *)buffer;
    for (int i = 0; i < 256; ++i) {
        if (write) {
            outw(ATA_DATA, words[i]);
        } else {
            words[i] = inw(ATA_DATA);
        }
    }

    return 0;
}

void disk_init(void) {
    /* Idempotent: disk_init() is reached from both storage_init() and
     * kernel_main(). Re-running virtio_blk_init() would reset the device and
     * allocate a second vring (leaking the first from the bump allocator) —
     * wasteful and a source of device-state races. Probe only once. */
    static int initialized = 0;
    if (initialized) {
        return;
    }
    initialized = 1;

    backend = DISK_BACKEND_NONE;
    ide_present = 0;

    virtio_blk_init();
    if (virtio_blk_present()) {
        backend = DISK_BACKEND_VIRTIO;
        return;
    }

    ide_probe();
    if (ide_present) {
        backend = DISK_BACKEND_IDE;
    }
}

int disk_present(void) {
    return backend != DISK_BACKEND_NONE;
}

const char *disk_backend_name(void) {
    switch (backend) {
    case DISK_BACKEND_VIRTIO:
        return virtio_blk_name();
    case DISK_BACKEND_IDE:
        return "IDE/ATA PIO";
    default:
        return "none";
    }
}

int disk_read(uint32_t lba, void *buffer) {
    if (backend == DISK_BACKEND_VIRTIO) {
        return virtio_blk_read(lba, buffer);
    }
    if (backend == DISK_BACKEND_IDE) {
        return ide_transfer(lba, buffer, 0);
    }
    return -1;
}

int disk_write(uint32_t lba, const void *buffer) {
    if (backend == DISK_BACKEND_VIRTIO) {
        return virtio_blk_write(lba, buffer);
    }
    if (backend == DISK_BACKEND_IDE) {
        return ide_transfer(lba, (void *)buffer, 1);
    }
    return -1;
}
