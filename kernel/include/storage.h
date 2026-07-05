#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>
#include <stdint.h>

#define VAULT_KEY_MAX  16
#define VAULT_VAL_MAX  64
#define VAULT_ENTRIES  8

void storage_init(void);
int storage_is_valid(void);
int storage_disk_present(void);

int storage_put(const char *key, const char *value);
const char *storage_get(const char *key);
int storage_copy_node(const char *key, char *out, size_t out_len);
void storage_list(void);

int storage_snapshot(void);
int storage_restore(void);
int storage_sync_disk(void);
int storage_load_disk(void);

void storage_export_serial(void);
int storage_import_serial(void);

#endif
