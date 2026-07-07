#include "console.h"
#include "dns.h"
#include "disc.h"
#include "disk.h"
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
#include "pkg.h"
#include "recognizer.h"
#include "speaker.h"
#include "tcp.h"
#include "program.h"
#include "sched.h"
#include "security.h"
#include "serial.h"
#include "irc_server.h"
#include "server.h"
#include "storage.h"
#include "syscall.h"
#include "timer.h"

void shell_run(void);

void kernel_main(void) {
    serial_init();
    serial_write("Grid OS 7.1.1 boot\n");
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
    disc_init();
    speaker_init();
    recognizer_init();
    net_init();
    dns_init();
    tcp_init();
    grid_server_init();
    grid_irc_server_init();

    log_event("Grid OS 7.1.1 boot");

    __asm__ volatile("sti");

    disk_init();
    gfs_init();
    pkg_init();

    shell_run();
}
