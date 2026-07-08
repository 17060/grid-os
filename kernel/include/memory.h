#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096UL

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITE   (1ULL << 1)
#define PAGE_USER    (1ULL << 2)
#define PAGE_NX      (1ULL << 63)

#define USER_CODE_VADDR 0x0000000000400000UL
#define USER_STACK_VADDR 0x0000000000700000UL
#define USER_STACK_PAGES 4

void memory_init(void);
void memory_map_kernel(void);

uint64_t *memory_create_user_tables(void);
void memory_destroy_user_tables(uint64_t *pml4);

/* Tag subsequent user page/table allocations with a program id (slot + 1);
 * memory_release_user reclaims every pool page tagged with that id when
 * the program slot is released. Pass 0 to clear the tag. */
void memory_user_set_owner(int owner);
void memory_release_user(int owner);

int memory_map_user_rx(uint64_t *pml4, uint64_t vaddr, const void *src, size_t len);
int memory_map_user_rw(uint64_t *pml4, uint64_t vaddr, size_t len, int zero_fill);
int memory_map_user_segment(uint64_t *pml4, uint64_t vaddr, const void *src,
                            size_t file_size, size_t mem_size, int writable, int executable);
void memory_switch_tables(uint64_t *pml4);
uint64_t *memory_current_tables(void);
uint64_t *memory_kernel_tables(void);

void memory_map_low_ram(void);
int memory_map_kernel_phys(uint64_t phys, size_t size);
void *memory_dma_alloc(size_t size, size_t align);

#endif
