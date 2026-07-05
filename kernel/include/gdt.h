#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x1B
#define USER_DS   0x23

void gdt_init(void);

#endif
