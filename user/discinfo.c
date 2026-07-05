#include "usys.h"

static void write_line(const char *text) {
    sys_write(text);
    sys_write("\n");
}

void _start(void) {
    char disc[65];
    char cycles[16];

    write_line("=== IDENTITY DISC RECORD ===");
    sys_disc(disc, 64);
    sys_write("Disc:   ");
    write_line(disc);
    sys_cycles(cycles, sizeof(cycles));
    sys_write("Cycles: ");
    write_line(cycles);
    write_line("Entity: Program (ring 3)");
    write_line("Status: derezz-resistant");
    sys_exit(0);
}
