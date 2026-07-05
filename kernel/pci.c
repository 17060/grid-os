#include "pci.h"

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA    0xCFCu

static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint32_t pci_address(const pci_device_t *dev, uint8_t offset) {
    return 0x80000000u | ((uint32_t)dev->bus << 16) | ((uint32_t)dev->slot << 11) |
           ((uint32_t)dev->func << 8) | (uint32_t)(offset & 0xFCu);
}

uint32_t pci_read32(const pci_device_t *dev, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, pci_address(dev, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read16(const pci_device_t *dev, uint8_t offset) {
    uint32_t value = pci_read32(dev, (uint8_t)(offset & 0xFCu));
    return (uint16_t)(value >> ((offset & 2u) * 8u));
}

uint8_t pci_read8(const pci_device_t *dev, uint8_t offset) {
    uint32_t value = pci_read32(dev, (uint8_t)(offset & 0xFCu));
    return (uint8_t)(value >> ((offset & 3u) * 8u));
}

void pci_write32(const pci_device_t *dev, uint8_t offset, uint32_t value) {
    outl(PCI_CONFIG_ADDRESS, pci_address(dev, offset));
    outl(PCI_CONFIG_DATA, value);
}

void pci_write16(const pci_device_t *dev, uint8_t offset, uint16_t value) {
    uint32_t current = pci_read32(dev, (uint8_t)(offset & 0xFCu));
    int shift = (offset & 2) * 8;
    uint32_t mask = 0xFFFFu << shift;
    pci_write32(dev, (uint8_t)(offset & 0xFCu), (current & ~mask) | ((uint32_t)value << shift));
}

void pci_write8(const pci_device_t *dev, uint8_t offset, uint8_t value) {
    uint32_t current = pci_read32(dev, (uint8_t)(offset & 0xFCu));
    int shift = (offset & 3) * 8;
    uint32_t mask = 0xFFu << shift;
    pci_write32(dev, (uint8_t)(offset & 0xFCu), (current & ~mask) | ((uint32_t)value << shift));
}

void pci_init(void) {
}

int pci_find_device(uint16_t vendor, uint16_t device, pci_device_t *out) {
    for (uint8_t bus = 0; bus < 4; ++bus) {
        for (uint8_t slot = 0; slot < 32; ++slot) {
            for (uint8_t func = 0; func < 8; ++func) {
                pci_device_t probe = {
                    .bus = bus,
                    .slot = slot,
                    .func = func,
                };

                uint32_t id = pci_read32(&probe, 0);
                if (id == 0xFFFFFFFFu || id == 0) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                probe.vendor = (uint16_t)(id & 0xFFFFu);
                probe.device = (uint16_t)(id >> 16);
                if (probe.vendor == vendor && probe.device == device) {
                    if (out) {
                        *out = probe;
                    }
                    return 0;
                }

                if (func == 0 && (pci_read8(&probe, 0x0E) & 0x80u) == 0) {
                    break;
                }
            }
        }
    }

    return -1;
}

uint64_t pci_get_bar(const pci_device_t *dev, int bar, int *is_io) {
    uint8_t offset = (uint8_t)(0x10 + bar * 4);
    uint32_t low = pci_read32(dev, offset);

    if (is_io) {
        *is_io = (int)(low & 1u);
    }

    if (low & 1u) {
        return (uint64_t)(low & 0xFFFFFFFCu);
    }

    uint64_t addr = (uint64_t)(low & 0xFFFFFFF0u);
    if ((low & 0x00000004u) != 0) {
        uint32_t high = pci_read32(dev, (uint8_t)(offset + 4));
        addr |= ((uint64_t)high << 32);
    }

    return addr;
}
