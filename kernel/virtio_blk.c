#include "memory.h"
#include "pci.h"
#include "virtio_blk.h"

#include <stddef.h>
#include <stdint.h>

#define VIRTIO_VENDOR_ID      0x1AF4u
#define VIRTIO_DEV_BLK_LEGACY 0x1001u
#define VIRTIO_DEV_BLK_MODERN 0x1042u

#define VIRTIO_PCI_CAP_VENDOR 0x09u
#define VIRTIO_PCI_CAP_COMMON 1u
#define VIRTIO_PCI_CAP_NOTIFY 2u

#define VIRTIO_STATUS_ACK         1u
#define VIRTIO_STATUS_DRIVER      2u
#define VIRTIO_STATUS_DRIVER_OK   4u
#define VIRTIO_STATUS_FEATURES_OK 8u

#define VIRTIO_F_VERSION_1        (1u << 0)

#define VIRTIO_BLK_T_IN  0u
#define VIRTIO_BLK_T_OUT 1u

#define VRING_DESC_F_NEXT  1u
#define VRING_DESC_F_WRITE 2u

#define VIRTIO_RING_SIZE   8u
#define VIRTIO_SECTOR_SIZE 512u

#define COMMON_QUEUE_SELECT 0x16u
#define COMMON_QUEUE_SIZE   0x18u
#define COMMON_QUEUE_ENABLE 0x1Cu
#define COMMON_QUEUE_NOTIFY 0x1Eu
#define COMMON_QUEUE_DESC   0x20u
#define COMMON_QUEUE_AVAIL  0x28u
#define COMMON_QUEUE_USED   0x30u
#define COMMON_DEVICE_STATUS 0x14u
#define COMMON_DRIVER_FEATURE 0x0Cu
#define COMMON_DRIVER_FEATURE_SEL 0x08u
#define COMMON_DEVICE_FEATURE 0x04u
#define COMMON_DEVICE_FEATURE_SEL 0x00u

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} vring_desc_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_RING_SIZE];
} vring_avail_t;

typedef struct {
    uint32_t id;
    uint32_t len;
} vring_used_elem_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    vring_used_elem_t ring[VIRTIO_RING_SIZE];
} vring_used_t;

typedef struct {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} virtio_blk_req_hdr_t;

typedef struct {
    int present;
    int modern;
    pci_device_t pci;
    uint16_t io_base;
    uint64_t mmio_base;
    uint64_t notify_base;
    uint32_t notify_multiplier;
    uint64_t common_offset;
    uint16_t queue_size;
    vring_desc_t *desc;
    vring_avail_t *avail;
    vring_used_t *used;
    uint8_t *req_status;
    uint8_t *data_buf;
    virtio_blk_req_hdr_t *req_hdr;
    uint16_t last_used_idx;
} virtio_blk_t;

static virtio_blk_t blk;

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

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint32_t mmio_read32(uint64_t base, uint32_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

static inline uint16_t mmio_read16(uint64_t base, uint32_t offset) {
    return *(volatile uint16_t *)(base + offset);
}

static inline void mmio_write32(uint64_t base, uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)(base + offset) = value;
}

static inline void mmio_write16(uint64_t base, uint32_t offset, uint16_t value) {
    *(volatile uint16_t *)(base + offset) = value;
}

static inline void mmio_write8(uint64_t base, uint32_t offset, uint8_t value) {
    *(volatile uint8_t *)(base + offset) = value;
}

static inline void mmio_write64(uint64_t base, uint32_t offset, uint64_t value) {
    mmio_write32(base, offset, (uint32_t)value);
    mmio_write32(base, offset + 4u, (uint32_t)(value >> 32));
}

static uint64_t common_base(void) {
    return blk.mmio_base + blk.common_offset;
}

static uint8_t pci_find_vendor_cap(const pci_device_t *dev, uint8_t cfg_type, uint8_t *bar_out,
                                   uint16_t *offset_out, uint32_t *extra_out) {
    if ((pci_read8(dev, 0x06) & 0x10u) == 0) {
        return 0;
    }

    uint8_t ptr = pci_read8(dev, 0x34);
    while (ptr != 0) {
        if (pci_read8(dev, ptr) == VIRTIO_PCI_CAP_VENDOR &&
            pci_read8(dev, (uint8_t)(ptr + 3)) == cfg_type) {
            if (bar_out) {
                *bar_out = pci_read8(dev, (uint8_t)(ptr + 4));
            }
            if (offset_out) {
                *offset_out = pci_read16(dev, (uint8_t)(ptr + 8));
            }
            if (extra_out) {
                *extra_out = pci_read32(dev, (uint8_t)(ptr + 12));
            }
            return ptr;
        }
        ptr = pci_read8(dev, (uint8_t)(ptr + 1));
    }

    return 0;
}

static void set_status(uint8_t value) {
    if (blk.modern) {
        mmio_write8(common_base(), COMMON_DEVICE_STATUS, value);
    } else {
        outb((uint16_t)(blk.io_base + 18u), value);
    }
}

static uint8_t read_status(void) {
    if (blk.modern) {
        return (uint8_t)mmio_read32(common_base(), COMMON_DEVICE_STATUS);
    }
    return inb((uint16_t)(blk.io_base + 18u));
}

static void device_reset(void) {
    set_status(0);
}

static int negotiate_features(void) {
    uint32_t features = 0;

    if (blk.modern) {
        mmio_write32(common_base(), COMMON_DEVICE_FEATURE_SEL, 0);
        features = mmio_read32(common_base(), COMMON_DEVICE_FEATURE);
        mmio_write32(common_base(), COMMON_DRIVER_FEATURE_SEL, 0);
        mmio_write32(common_base(), COMMON_DRIVER_FEATURE, features & VIRTIO_F_VERSION_1);
    } else {
        features = inl((uint16_t)(blk.io_base + 0u));
        outl((uint16_t)(blk.io_base + 4u), features & VIRTIO_F_VERSION_1);
    }

    set_status((uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK));
    return (read_status() & VIRTIO_STATUS_FEATURES_OK) != 0 ? 0 : -1;
}

static void notify_queue(void) {
    if (blk.modern) {
        mmio_write16(common_base(), COMMON_QUEUE_SELECT, 0);
        uint16_t queue_notify = mmio_read16(common_base(), COMMON_QUEUE_NOTIFY);
        mmio_write16(blk.notify_base, (uint32_t)queue_notify * blk.notify_multiplier, 0);
    } else {
        outw((uint16_t)(blk.io_base + 16u), 0);
    }
}

static int setup_queue(void) {
    uint16_t queue_size = VIRTIO_RING_SIZE;

    blk.desc = (vring_desc_t *)memory_dma_alloc(sizeof(vring_desc_t) * VIRTIO_RING_SIZE, PAGE_SIZE);
    blk.avail = (vring_avail_t *)memory_dma_alloc(sizeof(vring_avail_t), PAGE_SIZE);
    blk.used = (vring_used_t *)memory_dma_alloc(sizeof(vring_used_t), PAGE_SIZE);
    blk.req_hdr = (virtio_blk_req_hdr_t *)memory_dma_alloc(sizeof(virtio_blk_req_hdr_t), PAGE_SIZE);
    blk.data_buf = (uint8_t *)memory_dma_alloc(VIRTIO_SECTOR_SIZE, PAGE_SIZE);
    blk.req_status = (uint8_t *)memory_dma_alloc(1, 1);

    if (!blk.desc || !blk.avail || !blk.used || !blk.req_hdr || !blk.data_buf || !blk.req_status) {
        return -1;
    }

    blk.last_used_idx = 0;
    blk.avail->flags = 0;
    blk.avail->idx = 0;
    blk.used->flags = 0;
    blk.used->idx = 0;

    if (blk.modern) {
        mmio_write16(common_base(), COMMON_QUEUE_SELECT, 0);
        queue_size = mmio_read16(common_base(), COMMON_QUEUE_SIZE);
        if (queue_size == 0 || queue_size > VIRTIO_RING_SIZE) {
            queue_size = VIRTIO_RING_SIZE;
        }
        mmio_write16(common_base(), COMMON_QUEUE_SIZE, queue_size);
        mmio_write64(common_base(), COMMON_QUEUE_DESC, (uint64_t)(uintptr_t)blk.desc);
        mmio_write64(common_base(), COMMON_QUEUE_AVAIL, (uint64_t)(uintptr_t)blk.avail);
        mmio_write64(common_base(), COMMON_QUEUE_USED, (uint64_t)(uintptr_t)blk.used);
        mmio_write16(common_base(), COMMON_QUEUE_ENABLE, 1);
    } else {
        outw((uint16_t)(blk.io_base + 14u), 0);
        outw((uint16_t)(blk.io_base + 12u), queue_size);
        outl((uint16_t)(blk.io_base + 8u), (uint32_t)((uint64_t)(uintptr_t)blk.desc >> 12));
    }

    blk.queue_size = queue_size;
    return 0;
}

static int wait_used(uint16_t desc_idx) {
    uint16_t target = (uint16_t)(blk.last_used_idx + 1);

    for (int spin = 0; spin < 100000; ++spin) {
        if (blk.used->idx >= target) {
            vring_used_elem_t elem = blk.used->ring[blk.last_used_idx % VIRTIO_RING_SIZE];
            blk.last_used_idx++;
            if (elem.id != desc_idx) {
                return -1;
            }
            return blk.req_status[0] == 0 ? 0 : -1;
        }
    }

    return -1;
}

static int submit_request(uint32_t type, uint32_t lba, void *buffer, int write) {
    uint16_t data_flags = (uint16_t)(VRING_DESC_F_NEXT |
                                     (write ? 0 : VRING_DESC_F_WRITE));

    blk.req_hdr->type = type;
    blk.req_hdr->reserved = 0;
    blk.req_hdr->sector = lba;
    blk.req_status[0] = 0xFF;

    if (write) {
        for (size_t i = 0; i < VIRTIO_SECTOR_SIZE; ++i) {
            blk.data_buf[i] = ((const uint8_t *)buffer)[i];
        }
    }

    blk.desc[0].addr = (uint64_t)(uintptr_t)blk.req_hdr;
    blk.desc[0].len = sizeof(virtio_blk_req_hdr_t);
    blk.desc[0].flags = VRING_DESC_F_NEXT;
    blk.desc[0].next = 1;

    blk.desc[1].addr = (uint64_t)(uintptr_t)blk.data_buf;
    blk.desc[1].len = VIRTIO_SECTOR_SIZE;
    blk.desc[1].flags = data_flags;
    blk.desc[1].next = 2;

    blk.desc[2].addr = (uint64_t)(uintptr_t)blk.req_status;
    blk.desc[2].len = 1;
    blk.desc[2].flags = VRING_DESC_F_WRITE;
    blk.desc[2].next = 0;

    blk.avail->ring[blk.avail->idx % VIRTIO_RING_SIZE] = 0;
    blk.avail->idx++;

    notify_queue();
    return wait_used(0);
}

static int probe_modern(const pci_device_t *dev) {
    uint8_t bar = 0;
    uint8_t notify_bar = 0;
    uint16_t common_off = 0;
    uint16_t notify_off = 0;
    uint32_t notify_multiplier = 0;

    if (!pci_find_vendor_cap(dev, VIRTIO_PCI_CAP_COMMON, &bar, &common_off, 0)) {
        return -1;
    }
    if (!pci_find_vendor_cap(dev, VIRTIO_PCI_CAP_NOTIFY, &notify_bar, &notify_off,
                             &notify_multiplier)) {
        return -1;
    }

    int is_io = 0;
    uint64_t bar_addr = pci_get_bar(dev, bar, &is_io);
    uint64_t notify_addr = pci_get_bar(dev, notify_bar, &is_io);
    if (bar_addr == 0 || is_io) {
        return -1;
    }

    if (memory_map_kernel_phys(bar_addr, PAGE_SIZE) != 0) {
        return -1;
    }
    if (notify_addr != bar_addr && memory_map_kernel_phys(notify_addr, PAGE_SIZE) != 0) {
        return -1;
    }

    blk.modern = 1;
    blk.pci = *dev;
    blk.mmio_base = bar_addr;
    blk.common_offset = common_off;
    blk.notify_base = notify_addr;
    blk.notify_multiplier = notify_multiplier ? notify_multiplier : 4u;
    (void)notify_off;
    return 0;
}

static int probe_legacy(const pci_device_t *dev) {
    int is_io = 0;
    uint64_t bar = pci_get_bar(dev, 0, &is_io);
    if (!is_io || bar == 0) {
        return -1;
    }

    blk.modern = 0;
    blk.pci = *dev;
    blk.io_base = (uint16_t)bar;
    return 0;
}

void virtio_blk_init(void) {
    pci_device_t dev;
    uint8_t test_sector[VIRTIO_SECTOR_SIZE];

    blk.present = 0;
    blk.modern = 0;

    if (pci_find_device(VIRTIO_VENDOR_ID, VIRTIO_DEV_BLK_MODERN, &dev) == 0) {
        if (probe_modern(&dev) == 0) {
            goto setup;
        }
    }

    if (pci_find_device(VIRTIO_VENDOR_ID, VIRTIO_DEV_BLK_LEGACY, &dev) == 0) {
        if (probe_legacy(&dev) == 0) {
            goto setup;
        }
    }

    return;

setup:
    device_reset();
    set_status(VIRTIO_STATUS_ACK);
    set_status((uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER));
    if (negotiate_features() != 0) {
        device_reset();
        return;
    }
    if (setup_queue() != 0) {
        device_reset();
        return;
    }

    set_status((uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK |
                         VIRTIO_STATUS_DRIVER_OK));
    if ((read_status() & VIRTIO_STATUS_DRIVER_OK) == 0) {
        device_reset();
        return;
    }

    if (submit_request(VIRTIO_BLK_T_IN, 0, test_sector, 0) != 0) {
        device_reset();
        return;
    }

    blk.present = 1;
}

int virtio_blk_present(void) {
    return blk.present;
}

int virtio_blk_read(uint32_t lba, void *buffer) {
    if (!blk.present || !buffer) {
        return -1;
    }
    return submit_request(VIRTIO_BLK_T_IN, lba, buffer, 0);
}

int virtio_blk_write(uint32_t lba, const void *buffer) {
    if (!blk.present || !buffer) {
        return -1;
    }
    return submit_request(VIRTIO_BLK_T_OUT, lba, (void *)buffer, 1);
}

const char *virtio_blk_name(void) {
    if (!blk.present) {
        return "none";
    }
    return blk.modern ? "virtio-blk (PCI/MMIO)" : "virtio-blk (legacy PIO)";
}
