#ifndef PKG_H
#define PKG_H

#include <stddef.h>

void pkg_init(void);
void pkg_rescan(void);

int pkg_install_manifest(const char *manifest_path);
int pkg_remove(const char *name);
int pkg_recv_gridlink(void);

void pkg_list_packages(void);
void pkg_list_modules(void);
int pkg_info(const char *name);

int pkg_find_module(const char *name, char *path_out, size_t path_cap,
                    char *desc_out, size_t desc_cap);
int pkg_run_module(const char *name);
int pkg_load_module_path(const char *path);

void pkg_format_package_list(char *out, size_t cap);
void pkg_format_module_list(char *out, size_t cap);

#endif
