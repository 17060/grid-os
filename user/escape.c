#include "usys.h"

/*
 * escape — a deliberately malicious ring-3 program, for teaching.
 *
 * It tries to break out of the sandbox by writing to kernel memory. A correct
 * kernel maps kernel pages as supervisor-only, so a ring-3 write triggers a
 * CPU fault, the kernel catches it, and this program is killed before it can do
 * any damage. The final line should NEVER print — if it does, the sandbox is
 * broken.
 *
 * Spawn it from the shell:  spawn escape
 */
void _start(void) {
    sys_write("\n");
    sys_write("escape: I am a ring-3 (user-mode) program inside the sandbox.\n");
    sys_write("escape: I will now try to WRITE to kernel memory at 0x100000.\n");
    sys_write("escape: a correct kernel must stop me with a fault...\n");

    /* Kernel code/data lives at 1 MiB. It is mapped in the sandbox's page
     * tables (interrupt handlers need it) but as SUPERVISOR pages with no user
     * bit — so this store from ring 3 is a protection violation. */
    volatile unsigned long *kernel_mem = (unsigned long *)0x100000UL;
    *kernel_mem = 0xDEADBEEFUL;

    /* Unreachable if ring separation works: the store above faults and the
     * kernel terminates us. */
    sys_write("escape: !!! I wrote to kernel memory -- THE SANDBOX FAILED !!!\n");
    sys_exit(1);
}
