#ifndef SYSCALL_H
#define SYSCALL_H

#include "program.h"
#include <stdint.h>

#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_DISC        2
#define SYS_VAULT_GET   3
#define SYS_CYCLES      4
#define SYS_READ        5
#define SYS_READ_LINE   6
#define SYS_GRIDFS_READ 7

void syscall_init(void);
void syscall_set_active_program(grid_program_t *program);
grid_program_t *syscall_active_program(void);
uint64_t syscall_handler(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);

/* Recent-syscall trace, newest first (index 0 = most recent). Backs the
 * `syscalls` observability shell command. */
int syscall_trace_count(void);
const char *syscall_trace_name(int i);
uint64_t syscall_trace_arg(int i);
const char *syscall_trace_prog(int i);

#endif
