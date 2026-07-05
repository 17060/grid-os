#ifndef DISK_H
#define DISK_H

#include <stddef.h>
#include <stdint.h>

#define DISK_SECTOR_SIZE 512

void disk_init(void);
int disk_present(void);
const char *disk_backend_name(void);
int disk_read(uint32_t lba, void *buffer);
int disk_write(uint32_t lba, const void *buffer);

#endif
