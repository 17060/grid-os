#include "usys.h"

void _start(void) {
    char disc[65];

    sys_write("Grid program online in ring 3 sandbox.\n");
    sys_write("Program disc: ");
    sys_disc(disc, 64);
    sys_write(disc);
    sys_write("\n");
    sys_exit(0);
}
