#ifndef VIRTIO_BLK_H
#define VIRTIO_BLK_H

#include <stdint.h>

void virtio_blk_init(void);
int virtio_blk_present(void);
int virtio_blk_read(uint32_t lba, void *buffer);
int virtio_blk_write(uint32_t lba, const void *buffer);
const char *virtio_blk_name(void);

#endif
