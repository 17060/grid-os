#ifndef GRIDFS_H
#define GRIDFS_H

#include <stddef.h>

int gridfs_read(const char *path, char *out, size_t out_len);
void gridfs_list(const char *path);
int gridfs_copy_to_user(const char *path, char *out, size_t out_len);

#endif
