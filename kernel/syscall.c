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

/* ---- syscall trace ring buffer (for the `syscalls` observability command) ----
 * Records the most recent ring-3 -> kernel calls so a learner can watch what a
 * program asks the kernel to do. Only ring-3 programs use syscalls; the shell
 * and GridBASIC run in the kernel and call services directly, so the trace is
 * a clean view of sandboxed-program activity. */
#define SYS_TRACE_MAX 32
typedef struct {
    uint64_t num;
    uint64_t arg1;
    char prog[PROGRAM_NAME_MAX];
} sys_trace_t;
static sys_trace_t g_trace[SYS_TRACE_MAX];
static int g_trace_head = 0;   /* next slot to write */
static int g_trace_count = 0;  /* number of valid records (capped at MAX) */

static const char *sys_name(uint64_t n) {
    switch (n) {
    case SYS_EXIT:        return "exit";
    case SYS_WRITE:       return "write";
    case SYS_DISC:        return "disc";
    case SYS_VAULT_GET:   return "vault_get";
    case SYS_CYCLES:      return "cycles";
    case SYS_READ:        return "read";
    case SYS_READ_LINE:   return "read_line";
    case SYS_GRIDFS_READ: return "gridfs_read";
    default:              return "?";
    }
}

static void trace_record(uint64_t num, uint64_t arg1) {
    sys_trace_t *e = &g_trace[g_trace_head];
    e->num = num;
    e->arg1 = arg1;
    const char *pn = active_program ? active_program->name : "-";
    size_t i = 0;
    while (pn[i] && i + 1 < sizeof(e->prog)) { e->prog[i] = pn[i]; i++; }
    e->prog[i] = '\0';
    g_trace_head = (g_trace_head + 1) % SYS_TRACE_MAX;
    if (g_trace_count < SYS_TRACE_MAX) {
        g_trace_count++;
    }
}

/* index 0 = most recent */
static const sys_trace_t *trace_at(int i) {
    if (i < 0 || i >= g_trace_count) {
        return 0;
    }
    int idx = (g_trace_head - 1 - i + 2 * SYS_TRACE_MAX) % SYS_TRACE_MAX;
    return &g_trace[idx];
}

int syscall_trace_count(void) { return g_trace_count; }
const char *syscall_trace_name(int i) { const sys_trace_t *e = trace_at(i); return e ? sys_name(e->num) : ""; }
uint64_t syscall_trace_arg(int i) { const sys_trace_t *e = trace_at(i); return e ? e->arg1 : 0; }
const char *syscall_trace_prog(int i) { const sys_trace_t *e = trace_at(i); return e ? e->prog : ""; }

uint64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    trace_record(number, arg1);
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
