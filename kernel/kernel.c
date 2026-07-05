#include "console.h"
#include "grid.h"
#include "gfs.h"
#include "gdt.h"
#include "idt.h"
#include "iso.h"
#include "log.h"
#include "memory.h"
#include "mouse.h"
#include "net.h"
#include "pci.h"
#include "program.h"
#include "sched.h"
#include "security.h"
#include "serial.h"
#include "storage.h"
#include "syscall.h"
#include "timer.h"

void shell_run(void);

void kernel_main(void) {
    serial_init();
    serial_write("Grid OS 6.0 boot\n");
    console_init();
    gdt_init();
    memory_init();
    idt_init();
    timer_init();
    log_init();
    syscall_init();
    security_init();
    iso_zone_init();
    program_init();
    sched_init();
    pci_init();
    mouse_init();
    storage_init();
    gfs_init();
    net_init();

    log_event("Grid OS 6.0 boot");

    __asm__ volatile("sti");

    shell_run();
}
