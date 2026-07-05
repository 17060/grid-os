#include "elf.h"
#include "memory.h"

#include <stddef.h>
#include <stdint.h>

#define ELF_MAGIC0 0x7Fu
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'

#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ET_EXEC 2
#define PT_LOAD 1

#define PF_X 0x1u
#define PF_W 0x2u

typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint16_t e_phentsize;
    uint16_t e_phnum;
} elf64_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

static int is_flat_binary(const void *image, size_t size) {
    const uint8_t *bytes = (const uint8_t *)image;
    if (size < 4) {
        return 0;
    }
    return bytes[0] != ELF_MAGIC0;
}

int elf_is_valid(const void *image, size_t size) {
    const uint8_t *ident = (const uint8_t *)image;

    if (!image || size < sizeof(elf64_ehdr_t)) {
        return 0;
    }

    if (ident[0] != ELF_MAGIC0 || ident[1] != ELF_MAGIC1 ||
        ident[2] != ELF_MAGIC2 || ident[3] != ELF_MAGIC3) {
        return 0;
    }

    if (ident[4] != ELFCLASS64 || ident[5] != ELFDATA2LSB) {
        return 0;
    }

    const elf64_ehdr_t *hdr = (const elf64_ehdr_t *)image;
    if (hdr->e_type != ET_EXEC || hdr->e_machine != 0x3Eu) {
        return 0;
    }

    return 1;
}

int elf_load(uint64_t *pml4, const void *image, size_t size, uint64_t *entry_out) {
    if (!pml4 || !image || !entry_out || size == 0) {
        return -1;
    }

    if (is_flat_binary(image, size)) {
        if (memory_map_user_segment(pml4, USER_CODE_VADDR, image, size, size, 0, 1) != 0) {
            return -1;
        }
        *entry_out = USER_CODE_VADDR;
        return 0;
    }

    if (!elf_is_valid(image, size)) {
        return -1;
    }

    const elf64_ehdr_t *hdr = (const elf64_ehdr_t *)image;
    const uint8_t *base = (const uint8_t *)image;

    for (uint16_t i = 0; i < hdr->e_phnum; ++i) {
        size_t off = (size_t)hdr->e_phoff + (size_t)i * (size_t)hdr->e_phentsize;
        if (off + sizeof(elf64_phdr_t) > size) {
            return -1;
        }

        const elf64_phdr_t *ph = (const elf64_phdr_t *)(base + off);
        if (ph->p_type != PT_LOAD) {
            continue;
        }

        if (ph->p_offset + ph->p_filesz > size) {
            return -1;
        }

        int writable = (ph->p_flags & PF_W) ? 1 : 0;
        int executable = (ph->p_flags & PF_X) ? 1 : 0;
        const void *segment = base + ph->p_offset;

        if (memory_map_user_segment(pml4, ph->p_vaddr, segment, (size_t)ph->p_filesz,
                                    (size_t)ph->p_memsz, writable, executable) != 0) {
            return -1;
        }
    }

    *entry_out = hdr->e_entry;
    return 0;
}
