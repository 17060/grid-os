#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "program.h"
#include "syscall.h"

#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idt_pointer;

extern void isr_syscall(void);
extern void isr_gp_fault(void);
extern void isr_page_fault(void);
extern void isr_timer(void);

static void set_idt_gate(int vector, void (*handler)(void), uint8_t dpl) {
    uint64_t address = (uint64_t)handler;
    idt[vector].offset_low = (uint16_t)(address & 0xFFFF);
    idt[vector].selector = KERNEL_CS;
    idt[vector].ist = 0;
    idt[vector].type_attr = (uint8_t)(0x8E | (dpl << 5));
    idt[vector].offset_mid = (uint16_t)((address >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((address >> 32) & 0xFFFFFFFF);
    idt[vector].reserved = 0;
}

extern void idt_load(uint64_t ptr);

void idt_init(void) {
    for (int i = 0; i < 256; ++i) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].reserved = 0;
    }

    set_idt_gate(0x80, isr_syscall, 3);
    set_idt_gate(13, isr_gp_fault, 0);
    set_idt_gate(14, isr_page_fault, 0);
    set_idt_gate(32, isr_timer, 0);

    idt_pointer.limit = (uint16_t)(sizeof(idt) - 1);
    idt_pointer.base = (uint64_t)&idt;
    idt_load((uint64_t)&idt_pointer);
}

void idt_handle_gp_fault(void) {
    console_set_color(GRID_COL_ERROR);
    console_write_line("Program fault: general protection (sandbox boundary).");
    console_set_color(GRID_COL_DEFAULT);
    program_mark_fault();
}

void idt_handle_page_fault(void) {
    console_set_color(GRID_COL_ERROR);
    console_write_line("Program fault: page fault (W^X enforced).");
    console_set_color(GRID_COL_DEFAULT);
    program_mark_fault();
}
