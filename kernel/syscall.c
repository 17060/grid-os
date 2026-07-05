#include "console.h"
#include "gridfs.h"
#include "program.h"
#include "security.h"
#include "storage.h"
#include "syscall.h"

static grid_program_t *active_program = 0;

static int copy_to_user(char *dest, size_t cap, const char *src) {
    size_t i = 0;
    if (!dest || cap == 0) {
        return -1;
    }
    if (src) {
        while (src[i] && i + 1 < cap) {
            dest[i] = src[i];
            i++;
        }
    }
    dest[i] = '\0';
    return (int)i;
}

static int read_line_to_user(char *dest, size_t cap) {
    size_t length = 0;

    if (!dest || cap == 0) {
        return -1;
    }

    dest[0] = '\0';

    for (;;) {
        char c = console_read_char();

        if (c == '\n') {
            console_write_char('\n');
            dest[length] = '\0';
            return (int)length;
        }

        if (c == '\b') {
            if (length > 0) {
                length--;
                dest[length] = '\0';
            }
            continue;
        }

        if (length + 1 >= cap) {
            continue;
        }

        dest[length++] = c;
        dest[length] = '\0';
        console_write_char(c);
    }
}

void syscall_init(void) {
    active_program = 0;
}

void syscall_set_active_program(grid_program_t *program) {
    active_program = program;
}

grid_program_t *syscall_active_program(void) {
    return active_program;
}

static void write_user_buffer(const char *text) {
    if (!text) {
        return;
    }
    while (*text) {
        console_write_char(*text++);
    }
}

uint64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    switch (number) {
    case SYS_EXIT:
        if (active_program) {
            active_program->state = PROGRAM_EXITED;
            active_program->exit_code = (int)arg1;
        }
        program_request_return();
        return 0;
    case SYS_WRITE:
        if (active_program &&
            (active_program->capabilities & CAP_WRITE_GRID) == CAP_WRITE_GRID) {
            write_user_buffer((const char *)arg1);
        }
        return 0;
    case SYS_DISC:
        if (active_program) {
            program_format_disc_active((char *)arg1, (size_t)arg2);
        }
        return 0;
    case SYS_VAULT_GET:
        if (active_program &&
            (active_program->capabilities & CAP_READ_GRID) == CAP_READ_GRID) {
            return (uint64_t)storage_copy_node((const char *)arg1, (char *)arg2, (size_t)arg3);
        }
        return (uint64_t)-1;
    case SYS_CYCLES: {
        char buffer[16];
        uint32_t cycles = security_cycles();
        size_t pos = 0;

        if (cycles == 0) {
            buffer[pos++] = '0';
        } else {
            char tmp[16];
            size_t tlen = 0;
            while (cycles > 0) {
                tmp[tlen++] = (char)('0' + (cycles % 10));
                cycles /= 10;
            }
            while (tlen > 0) {
                buffer[pos++] = tmp[--tlen];
            }
        }
        buffer[pos] = '\0';
        return (uint64_t)copy_to_user((char *)arg1, (size_t)arg2, buffer);
    }
    case SYS_READ:
        if (!active_program) {
            return (uint64_t)-1;
        }
        return (uint64_t)(unsigned char)console_read_char();
    case SYS_READ_LINE:
        if (!active_program) {
            return (uint64_t)-1;
        }
        return (uint64_t)read_line_to_user((char *)arg1, (size_t)arg2);
    case SYS_GRIDFS_READ:
        if (active_program &&
            (active_program->capabilities & CAP_READ_GRID) == CAP_READ_GRID) {
            return (uint64_t)gridfs_copy_to_user((const char *)arg1, (char *)arg2, (size_t)arg3);
        }
        return (uint64_t)-1;
    default:
        return (uint64_t)-1;
    }
}
