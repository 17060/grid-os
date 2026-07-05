#include "usys.h"

#define LINE_MAX 80

static int equals(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static void write_line(const char *text) {
    sys_write(text);
    sys_write("\n");
}

void _start(void) {
    char line[LINE_MAX];
    char disc[65];
    char cycles[16];
    char value[64];

    write_line("");
    write_line("  +--------------------------------------+");
    write_line("  |  GRID SHELL  //  Flynn's Frontier   |");
    write_line("  +--------------------------------------+");
    write_line("Type 'help'. End of line with 'exit'.");
    write_line("");

    for (;;) {
        sys_write("gridsh> ");
        if (sys_read_line(line, sizeof(line)) < 0) {
            continue;
        }

        if (equals(line, "exit") || equals(line, "quit")) {
            write_line("End of line.");
            sys_exit(0);
        }
        if (equals(line, "help")) {
            write_line("help disc cycles motd cat exit");
            continue;
        }
        if (equals(line, "disc")) {
            sys_disc(disc, 64);
            sys_write("Disc: ");
            write_line(disc);
            continue;
        }
        if (equals(line, "cycles")) {
            sys_cycles(cycles, sizeof(cycles));
            sys_write("Cycles: ");
            write_line(cycles);
            continue;
        }
        if (equals(line, "motd")) {
            sys_vault_get("motd", value, sizeof(value));
            if (value[0] == '\0') {
                sys_gridfs_read("/motd", value, sizeof(value));
            }
            sys_write("Motd: ");
            write_line(value[0] ? value : "The Grid is open.");
            continue;
        }
        if (line[0] == 'c' && line[1] == 'a' && line[2] == 't' && line[3] == ' ') {
            const char *path = line + 4;
            if (sys_gridfs_read(path, value, sizeof(value)) == 0) {
                write_line(value);
            } else {
                write_line("GridFS: not found");
            }
            continue;
        }

        write_line("Unknown command. Try 'help'.");
    }
}
