#ifndef ELF_H
#define ELF_H

#include <stddef.h>
#include <stdint.h>

int elf_is_valid(const void *image, size_t size);
int elf_load(uint64_t *pml4, const void *image, size_t size, uint64_t *entry_out);

#endif
