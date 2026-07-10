#ifndef GFS_H
#define GFS_H

#include <stddef.h>
#include <stdint.h>

#define GFS_PATH_MAX 56

#define GFS_FILE_MAX 65536u

void gfs_init(void);
int gfs_present(void);
int gfs_format(void);

int gfs_read_file(const char *path, void *out, size_t out_cap, size_t *out_len);
int gfs_write_file(const char *path, const void *data, size_t size);
int gfs_delete_file(const char *path);

void gfs_list(const char *prefix);
int gfs_list_paths(const char *prefix, char paths[][GFS_PATH_MAX], int max_paths);
void gfs_print_status(void);

int gfs_seed_defaults(void);

/* On-disk geometry (512-byte sectors), for the `disklayout` observability
 * command — where the superblock, inode table, and file data physically sit. */
typedef struct {
    uint32_t super_lba;
    uint32_t inode_lba;
    uint32_t inode_sectors;
    uint32_t inode_max;
    uint32_t data_base_lba;
    uint32_t sectors_per_file;
    int mounted;
} gfs_layout_t;
void gfs_get_layout(gfs_layout_t *out);

#endif
