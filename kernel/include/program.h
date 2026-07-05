#ifndef PROGRAM_H
#define PROGRAM_H

#include <stddef.h>
#include <stdint.h>

#define PROGRAM_SLOTS 4
#define PROGRAM_NAME_MAX 16
#define PROGRAM_DISC_BYTES 16

typedef enum {
    PROGRAM_IDLE = 0,
    PROGRAM_RUNNING = 1,
    PROGRAM_EXITED = 2,
    PROGRAM_FAULT = 3
} program_state_t;

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rdx, rcx, rbx, rax;
    uint64_t rip, rflags, rsp;
} user_ctx_t;

typedef struct {
    int used;
    char name[PROGRAM_NAME_MAX];
    uint8_t disc[PROGRAM_DISC_BYTES];
    uint32_t capabilities;
    program_state_t state;
    int exit_code;
    uint64_t entry_point;
    uint64_t *page_tables;
    user_ctx_t ctx;
    int has_ctx;
    int preempted;
    uint64_t preempt_count;
} grid_program_t;

void program_init(void);
int program_count(void);
int program_spawn(const char *name, const void *image, size_t image_size, uint64_t entry_offset);
int program_spawn_gridprog(const char *name);
int program_spawn_named(const char *name);
int program_spawn_from_disk(const char *name);
int program_load_named(const char *name);
int program_run(int id);
int program_run_background(const char *name);
void program_preempt_return(void);
void program_save_context(const uint64_t *regs);
void program_print_catalog(void);
const grid_program_t *program_get(int id);
void program_format_disc(int id, char *out, size_t out_len);
void program_format_disc_active(char *out, size_t out_len);
void program_mark_fault(void);
void program_request_return(void);
void program_check_return(void);
void program_print_list(void);
void program_release(int id);

#endif
