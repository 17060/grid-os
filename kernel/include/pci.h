#ifndef PCI_H
#define PCI_H

#include <stdint.h>

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor;
    uint16_t device;
} pci_device_t;

void pci_init(void);
int pci_find_device(uint16_t vendor, uint16_t device, pci_device_t *out);
uint32_t pci_read32(const pci_device_t *dev, uint8_t offset);
uint16_t pci_read16(const pci_device_t *dev, uint8_t offset);
uint8_t pci_read8(const pci_device_t *dev, uint8_t offset);
void pci_write32(const pci_device_t *dev, uint8_t offset, uint32_t value);
void pci_write16(const pci_device_t *dev, uint8_t offset, uint16_t value);
void pci_write8(const pci_device_t *dev, uint8_t offset, uint8_t value);
uint64_t pci_get_bar(const pci_device_t *dev, int bar, int *is_io);

#endif
