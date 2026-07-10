#include "gdt.h"
#include "memory.h"

#include <stdint.h>

static uint64_t kernel_pml4[512] __attribute__((aligned(PAGE_SIZE)));
static uint64_t kernel_pdpt[512] __attribute__((aligned(PAGE_SIZE)));
static uint64_t kernel_pd[512] __attribute__((aligned(PAGE_SIZE)));
static uint64_t kernel_extra_pd[4][512] __attribute__((aligned(PAGE_SIZE)));
static uint64_t kernel_extra_pt[8][512] __attribute__((aligned(PAGE_SIZE)));

static int kernel_extra_pd_used = 0;
static int kernel_extra_pt_used = 0;

static uint8_t dma_pool[131072] __attribute__((aligned(PAGE_SIZE)));
static size_t dma_pool_used = 0;

static uint64_t *active_pml4 = kernel_pml4;

#define MAX_USER_PT   24
#define MAX_USER_DATA 16

static uint64_t user_pt_pool[MAX_USER_PT][512] __attribute__((aligned(PAGE_SIZE)));
static uint64_t user_data_pool[MAX_USER_DATA][512] __attribute__((aligned(PAGE_SIZE)));
/* Owner tag per pool page: 0 = free, otherwise the program id (slot + 1)
 * set via memory_user_set_owner. Pages are reclaimed by memory_release_user
 * when a program slot is released, so repeated spawns do not exhaust the
 * pools. -1 marks an allocation made with no owner configured. */
static int user_pt_owner[MAX_USER_PT];
static int user_data_owner[MAX_USER_DATA];
static int user_alloc_owner = 0;

static inline void load_cr3(uint64_t value) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(value) : "memory");
}

static inline void invlpg(void *addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static void zero_page(uint64_t *page) {
    for (size_t i = 0; i < 512; ++i) {
        page[i] = 0;
    }
}

static void enable_nxe(void) {
    /* "=A" does not bind edx:eax on x86-64 — use explicit registers so the
     * EFER write does not put garbage in the reserved high half (#GP). */
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000080));
    lo |= (1u << 11);
    __asm__ volatile("wrmsr" : : "c"(0xC0000080), "a"(lo), "d"(hi));
}

static void set_page_entry(uint64_t *table, size_t index, uint64_t phys, uint64_t flags) {
    table[index] = (phys & 0x000FFFFFFFFFF000ULL) | flags;
}

void memory_map_kernel(void) {
    zero_page(kernel_pml4);
    zero_page(kernel_pdpt);
    zero_page(kernel_pd);

    set_page_entry(kernel_pml4, 0, (uint64_t)kernel_pdpt, PAGE_PRESENT | PAGE_WRITE);
    set_page_entry(kernel_pdpt, 0, (uint64_t)kernel_pd, PAGE_PRESENT | PAGE_WRITE);
    set_page_entry(kernel_pd, 0, 0, PAGE_PRESENT | PAGE_WRITE | (1ULL << 7));
}

void memory_map_low_ram(void) {
    for (int i = 1; i < 64; ++i) {
        set_page_entry(kernel_pd, (size_t)i, (uint64_t)i * 0x200000ULL,
                       PAGE_PRESENT | PAGE_WRITE | (1ULL << 7));
    }
}

static uint64_t *alloc_kernel_pt(void) {
    if (kernel_extra_pt_used >= 8) {
        return 0;
    }
    uint64_t *page = kernel_extra_pt[kernel_extra_pt_used++];
    zero_page(page);
    return page;
}

static uint64_t *ensure_kernel_pdpt_entry(size_t pdpt_i) {
    if (!(kernel_pml4[0] & PAGE_PRESENT)) {
        return 0;
    }

    if (!(kernel_pdpt[pdpt_i] & PAGE_PRESENT)) {
        if (kernel_extra_pd_used >= 4) {
            return 0;
        }
        uint64_t *pd = kernel_extra_pd[kernel_extra_pd_used++];
        zero_page(pd);
        set_page_entry(kernel_pdpt, pdpt_i, (uint64_t)pd, PAGE_PRESENT | PAGE_WRITE);
    }

    return (uint64_t *)(kernel_pdpt[pdpt_i] & 0x000FFFFFFFFFF000ULL);
}

int memory_map_kernel_phys(uint64_t phys, size_t size) {
    uint64_t end = phys + size;

    while (phys < end) {
        uint64_t vaddr = phys;
        size_t pdpt_i = (vaddr >> 30) & 0x1FF;
        size_t pd_i = (vaddr >> 21) & 0x1FF;
        size_t pt_i = (vaddr >> 12) & 0x1FF;
        uint64_t *pd = ensure_kernel_pdpt_entry(pdpt_i);

        if (!pd) {
            return -1;
        }

        if (pd[pd_i] & PAGE_PRESENT) {
            if (pd[pd_i] & (1ULL << 7)) {
                return -1;
            }
        } else {
            uint64_t *pt = alloc_kernel_pt();
            if (!pt) {
                return -1;
            }
            set_page_entry(pd, pd_i, (uint64_t)pt, PAGE_PRESENT | PAGE_WRITE);
        }

        uint64_t *pt = (uint64_t *)(pd[pd_i] & 0x000FFFFFFFFFF000ULL);
        set_page_entry(pt, pt_i, phys, PAGE_PRESENT | PAGE_WRITE | PAGE_NX);
        invlpg((void *)vaddr);
        phys += PAGE_SIZE;
    }

    return 0;
}

void *memory_dma_alloc(size_t size, size_t align) {
    if (align == 0) {
        align = 16;
    }

    size_t aligned = (dma_pool_used + align - 1) & ~(align - 1);
    /* Overflow-safe bounds check: a huge `size` would make `aligned + size`
     * wrap below sizeof(dma_pool) and slip past the guard. */
    if (aligned > sizeof(dma_pool) || size > sizeof(dma_pool) - aligned) {
        return 0;
    }

    void *ptr = dma_pool + aligned;
    dma_pool_used = aligned + size;
    for (size_t i = aligned; i < dma_pool_used; ++i) {
        dma_pool[i] = 0;
    }
    return ptr;
}

void memory_get_stats(memory_stats_t *out) {
    if (!out) {
        return;
    }
    out->dma_used = dma_pool_used;
    out->dma_total = sizeof(dma_pool);
    out->user_pt_total = MAX_USER_PT;
    out->user_data_total = MAX_USER_DATA;
    int pt = 0, dt = 0;
    for (int i = 0; i < MAX_USER_PT; ++i) {
        if (user_pt_owner[i] != 0) pt++;
    }
    for (int i = 0; i < MAX_USER_DATA; ++i) {
        if (user_data_owner[i] != 0) dt++;
    }
    out->user_pt_used = pt;
    out->user_data_used = dt;
}

/* Physical address of the kernel PML4, read by the interrupt entry stubs
 * (interrupts.s). Handlers must switch to the full kernel mapping on entry:
 * user page tables only identity-map 0-4 MB of kernel memory, and kernel
 * globals above that (e.g. late BSS) are unreachable while user CR3 is
 * active. Set before idt_init()/sti, so it is valid for every interrupt. */
uint64_t kernel_cr3_phys;

void memory_init(void) {
    enable_nxe();
    memory_map_kernel();
    memory_map_low_ram();
    load_cr3((uint64_t)kernel_pml4);
    active_pml4 = kernel_pml4;
    kernel_cr3_phys = (uint64_t)kernel_pml4;
}

uint64_t *memory_kernel_tables(void) {
    return kernel_pml4;
}

uint64_t *memory_current_tables(void) {
    return active_pml4;
}

void memory_switch_tables(uint64_t *pml4) {
    active_pml4 = pml4;
    load_cr3((uint64_t)pml4);
}

static uint64_t *alloc_user_pt(void) {
    for (int i = 0; i < MAX_USER_PT; ++i) {
        if (user_pt_owner[i] == 0) {
            user_pt_owner[i] = user_alloc_owner ? user_alloc_owner : -1;
            zero_page(user_pt_pool[i]);
            return user_pt_pool[i];
        }
    }
    return 0;
}

static uint64_t *alloc_user_data(void) {
    for (int i = 0; i < MAX_USER_DATA; ++i) {
        if (user_data_owner[i] == 0) {
            user_data_owner[i] = user_alloc_owner ? user_alloc_owner : -1;
            zero_page(user_data_pool[i]);
            return user_data_pool[i];
        }
    }
    return 0;
}

void memory_user_set_owner(int owner) {
    user_alloc_owner = owner;
}

void memory_release_user(int owner) {
    if (owner == 0) {
        return;
    }
    for (int i = 0; i < MAX_USER_PT; ++i) {
        if (user_pt_owner[i] == owner) {
            user_pt_owner[i] = 0;
        }
    }
    for (int i = 0; i < MAX_USER_DATA; ++i) {
        if (user_data_owner[i] == owner) {
            user_data_owner[i] = 0;
        }
    }
}

uint64_t *memory_create_user_tables(void) {
    uint64_t *user_pml4 = alloc_user_pt();
    uint64_t *user_pdpt = alloc_user_pt();
    uint64_t *user_pd = alloc_user_pt();
    if (!user_pml4 || !user_pdpt || !user_pd) {
        return 0;
    }

    set_page_entry(user_pml4, 0, (uint64_t)user_pdpt, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    set_page_entry(user_pdpt, 0, (uint64_t)user_pd, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    /* Supervisor identity map of 0–4 MB so kernel text/data/bss (1 MB..~2.4 MB)
     * stays reachable for interrupts and syscalls while user CR3 is active.
     * USER_CODE_VADDR (4 MB) starts at pd[2], so nothing user-visible here. */
    set_page_entry(user_pd, 0, 0, PAGE_PRESENT | PAGE_WRITE | (1ULL << 7));
    set_page_entry(user_pd, 1, 0x200000ULL, PAGE_PRESENT | PAGE_WRITE | (1ULL << 7));

    return user_pml4;
}

void memory_destroy_user_tables(uint64_t *pml4) {
    (void)pml4;
}

static int map_pt_flags(uint64_t *pml4, uint64_t vaddr, uint64_t phys, uint64_t flags) {
    size_t pml4_i = (vaddr >> 39) & 0x1FF;
    size_t pdpt_i = (vaddr >> 30) & 0x1FF;
    size_t pd_i = (vaddr >> 21) & 0x1FF;
    size_t pt_i = (vaddr >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) {
        return -1;
    }

    uint64_t *pdpt = (uint64_t *)(pml4[pml4_i] & 0x000FFFFFFFFFF000ULL);
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_i] & 0x000FFFFFFFFFF000ULL);
    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) {
        return -1;
    }

    if (pd[pd_i] & (1ULL << 7)) {
        return -2;
    }

    uint64_t *pt = (uint64_t *)(pd[pd_i] & 0x000FFFFFFFFFF000ULL);
    if (!(pd[pd_i] & PAGE_PRESENT)) {
        pt = alloc_user_pt();
        if (!pt) {
            return -1;
        }
        set_page_entry(pd, pd_i, (uint64_t)pt, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    set_page_entry(pt, pt_i, phys, flags | PAGE_PRESENT | PAGE_USER);
    invlpg((void *)vaddr);
    return 0;
}

int memory_map_user_rx(uint64_t *pml4, uint64_t vaddr, const void *src, size_t len) {
    const uint8_t *bytes = (const uint8_t *)src;
    uint64_t addr = vaddr;

    while (len > 0) {
        uint64_t *page = alloc_user_data();
        if (!page) {
            return -1;
        }

        size_t chunk = PAGE_SIZE;
        if (chunk > len) {
            chunk = len;
        }

        for (size_t i = 0; i < PAGE_SIZE; ++i) {
            ((uint8_t *)page)[i] = (i < chunk) ? bytes[i] : 0;
        }

        /* RX = read + execute, W^X: no PAGE_WRITE and no PAGE_NX */
        if (map_pt_flags(pml4, addr, (uint64_t)page, 0) != 0) {
            return -1;
        }

        bytes += chunk;
        addr += PAGE_SIZE;
        len -= chunk;
    }

    return 0;
}

int memory_map_user_segment(uint64_t *pml4, uint64_t vaddr, const void *src,
                           size_t file_size, size_t mem_size, int writable, int executable) {
    const uint8_t *bytes = (const uint8_t *)src;
    uint64_t addr = vaddr;
    size_t mapped = 0;
    uint64_t flags = executable ? 0 : PAGE_NX;
    if (writable) {
        flags |= PAGE_WRITE;
    }

    if (writable && executable) {
        return -1;
    }

    if (mem_size < file_size) {
        return -1;
    }

    while (mapped < mem_size) {
        uint64_t *page = alloc_user_data();
        if (!page) {
            return -1;
        }

        for (size_t i = 0; i < PAGE_SIZE; ++i) {
            size_t offset = mapped + i;
            if (src && offset < file_size) {
                ((uint8_t *)page)[i] = bytes[offset];
            } else {
                ((uint8_t *)page)[i] = 0;
            }
        }

        if (map_pt_flags(pml4, addr, (uint64_t)page, flags) != 0) {
            return -1;
        }

        addr += PAGE_SIZE;
        mapped += PAGE_SIZE;
    }

    return 0;
}

int memory_map_user_rw(uint64_t *pml4, uint64_t vaddr, size_t len, int zero_fill) {
    uint64_t addr = vaddr;

    while (len > 0) {
        uint64_t *page = alloc_user_data();
        if (!page) {
            return -1;
        }

        if (!zero_fill) {
            return -1;
        }

        if (map_pt_flags(pml4, addr, (uint64_t)page, PAGE_WRITE | PAGE_NX) != 0) {
            return -1;
        }

        addr += PAGE_SIZE;
        if (len > PAGE_SIZE) {
            len -= PAGE_SIZE;
        } else {
            len = 0;
        }
    }

    return 0;
}
