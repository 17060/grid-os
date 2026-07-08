#include "gdt.h"

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} __attribute__((packed));

static struct gdt_entry gdt[7];
static struct tss_entry tss;
static struct gdt_ptr gdt_pointer;

extern void gdt_load(uint64_t gdt_ptr);
extern void tss_load(uint16_t selector);

static void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[index].base_low = (uint16_t)(base & 0xFFFF);
    gdt[index].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[index].access = access;
    gdt[index].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt[index].granularity |= gran;
    gdt[index].base_high = (uint8_t)((base >> 24) & 0xFF);
}

static void set_tss_entry(int index, uint64_t base, uint32_t limit) {
    gdt[index].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[index].base_low = (uint16_t)(base & 0xFFFF);
    gdt[index].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt[index].access = 0x89;
    gdt[index].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt[index].base_high = (uint8_t)((base >> 24) & 0xFF);
    gdt[index].granularity |= 0x00;

    gdt[index + 1].limit_low = (uint16_t)((base >> 32) & 0xFFFF);
    gdt[index + 1].base_low = (uint16_t)((base >> 48) & 0xFFFF);
    gdt[index + 1].base_middle = 0;
    gdt[index + 1].access = 0;
    gdt[index + 1].granularity = 0;
    gdt[index + 1].base_high = 0;
}

extern uint8_t stack_top[];

/* Dedicated ring-0 entry stack. TSS.rsp0 must NOT point at the main kernel
 * stack: while a ring-3 program runs, the kernel call chain that entered
 * user mode (enter_usermode's saved callee registers and return address,
 * plus every caller frame) is still live on the main stack, and a syscall
 * or interrupt arriving with rsp0 = stack_top would push its frames right
 * down through that live data. Keep this below 4 MB (early BSS) so it stays
 * inside the supervisor identity map of the user page tables. */
static uint8_t ring0_stack[16384] __attribute__((aligned(16)));

void gdt_init(void) {
    set_gdt_entry(0, 0, 0, 0, 0);
    set_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);
    set_gdt_entry(2, 0, 0xFFFFF, 0x92, 0xA0);
    set_gdt_entry(3, 0, 0xFFFFF, 0xFA, 0xA0);
    set_gdt_entry(4, 0, 0xFFFFF, 0xF2, 0xA0);
    set_tss_entry(5, (uint64_t)&tss, sizeof(tss) - 1);

    tss.rsp0 = (uint64_t)(ring0_stack + sizeof(ring0_stack));

    gdt_pointer.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_pointer.base = (uint64_t)&gdt;

    gdt_load((uint64_t)&gdt_pointer);
    tss_load(0x28);
}
