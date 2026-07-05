#include "console.h"
#include "elf.h"
#include "gfs.h"
#include "gdt.h"
#include "log.h"
#include "memory.h"
#include "program.h"
#include "sched.h"
#include "security.h"
#include "syscall.h"

#include <stddef.h>
#include <stdint.h>

extern void enter_usermode(uint64_t rip, uint64_t rsp, uint64_t pml4);
extern void enter_usermode_resume(const void *ctx, uint64_t pml4);
extern void enter_usermode_return(void);

static grid_program_t programs[PROGRAM_SLOTS];
#define PROGRAM_IMAGE_MAX 16384

static volatile int return_requested = 0;
static volatile int fault_occurred = 0;

extern const uint8_t gridprog_bin[];
extern const uint8_t gridprog_bin_end[];
extern const uint8_t discinfo_bin[];
extern const uint8_t discinfo_bin_end[];
extern const uint8_t gridsh_bin[];
extern const uint8_t gridsh_bin_end[];
extern const uint8_t lightcycle_bin[];
extern const uint8_t lightcycle_bin_end[];
extern const uint8_t gridloop_bin[];
extern const uint8_t gridloop_bin_end[];

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void derive_program_disc(grid_program_t *program, int slot) {
    const grid_identity_t *parent = security_current_identity();
    uint32_t state = 0x50524F47 ^ (uint32_t)(slot + 1);

    for (size_t i = 0; i < PROGRAM_DISC_BYTES; i += 4) {
        uint32_t block = xorshift32(&state);
        block ^= *(const uint32_t *)(parent->disc + i);
        program->disc[i] = (uint8_t)(block & 0xFF);
        program->disc[i + 1] = (uint8_t)((block >> 8) & 0xFF);
        program->disc[i + 2] = (uint8_t)((block >> 16) & 0xFF);
        program->disc[i + 3] = (uint8_t)((block >> 24) & 0xFF);
    }
}

static void copy_name(grid_program_t *program, const char *name) {
    size_t i = 0;
    if (name) {
        while (name[i] && i + 1 < PROGRAM_NAME_MAX) {
            program->name[i] = name[i];
            i++;
        }
    }
    if (i == 0) {
        program->name[0] = 'p';
        program->name[1] = 'r';
        program->name[2] = 'o';
        program->name[3] = 'g';
        program->name[4] = '-';
        program->name[5] = (char)('0' + (program - programs + 1));
        program->name[6] = '\0';
        return;
    }
    program->name[i] = '\0';
}

static int find_slot(void) {
    for (int i = 0; i < PROGRAM_SLOTS; ++i) {
        if (!programs[i].used) {
            return i;
        }
    }
    return -1;
}

static int program_enter(grid_program_t *program, uint64_t entry);

void program_init(void) {
    for (int i = 0; i < PROGRAM_SLOTS; ++i) {
        programs[i].used = 0;
        programs[i].page_tables = 0;
        programs[i].has_ctx = 0;
        programs[i].preempted = 0;
    }
    return_requested = 0;
    fault_occurred = 0;
}

int program_count(void) {
    int count = 0;
    for (int i = 0; i < PROGRAM_SLOTS; ++i) {
        if (programs[i].used) {
            count++;
        }
    }
    return count;
}

const grid_program_t *program_get(int id) {
    if (id < 1 || id > PROGRAM_SLOTS || !programs[id - 1].used) {
        return 0;
    }
    return &programs[id - 1];
}

void program_format_disc(int id, char *out, size_t out_len) {
    const grid_program_t *program = program_get(id);
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = 0;

    if (!program || out_len == 0) {
        return;
    }

    for (size_t i = 0; i < PROGRAM_DISC_BYTES; ++i) {
        if (pos + 2 >= out_len) {
            break;
        }
        out[pos++] = hex[(program->disc[i] >> 4) & 0xF];
        out[pos++] = hex[program->disc[i] & 0xF];
    }

    if (pos < out_len) {
        out[pos] = '\0';
    } else {
        out[out_len - 1] = '\0';
    }
}

void program_format_disc_active(char *out, size_t out_len) {
    for (int i = 0; i < PROGRAM_SLOTS; ++i) {
        if (programs[i].used && programs[i].state == PROGRAM_RUNNING) {
            program_format_disc(i + 1, out, out_len);
            return;
        }
    }
    if (out_len > 0) {
        out[0] = '\0';
    }
}

void program_mark_fault(void) {
    fault_occurred = 1;
    return_requested = 1;
    grid_program_t *p = syscall_active_program();
    if (p) {
        p->state = PROGRAM_FAULT;
        p->has_ctx = 0;
        p->preempted = 0;
    }
}

void program_preempt_return(void) {
    /* Legacy entry point. True preemption is now driven by the timer ISR,
       which saves user context and returns to the kernel directly. */
}

void program_save_context(const uint64_t *regs) {
    grid_program_t *p = syscall_active_program();
    if (!p) {
        return;
    }
    p->ctx.r15 = regs[0];
    p->ctx.r14 = regs[1];
    p->ctx.r13 = regs[2];
    p->ctx.r12 = regs[3];
    p->ctx.r11 = regs[4];
    p->ctx.r10 = regs[5];
    p->ctx.r9  = regs[6];
    p->ctx.r8  = regs[7];
    /* isr_timer push order puts rbp below rdi/rsi on the stack */
    p->ctx.rbp = regs[8];
    p->ctx.rdi = regs[9];
    p->ctx.rsi = regs[10];
    p->ctx.rdx = regs[11];
    p->ctx.rcx = regs[12];
    p->ctx.rbx = regs[13];
    p->ctx.rax = regs[14];
    p->ctx.rip    = regs[15];
    p->ctx.rflags = regs[17];
    p->ctx.rsp    = regs[18];
    p->has_ctx = 1;
    p->preempted = 1;
    p->preempt_count++;
}

void program_request_return(void) {
    return_requested = 1;
}

static void finish_return(void) {
    memory_switch_tables(memory_kernel_tables());
    grid_program_t *p = syscall_active_program();
    if (p && p->state == PROGRAM_RUNNING) {
        p->state = PROGRAM_EXITED;
        p->has_ctx = 0;
        p->preempted = 0;
    }
    syscall_set_active_program(0);
    enter_usermode_return();
}

void program_check_return(void) {
    /* Only unwind to the kernel caller when a user program is actually
     * active; a stray kernel-mode fault must not jump through a stale
     * kernel_return_rsp (that turns one fault into an endless loop). */
    if (return_requested && syscall_active_program()) {
        finish_return();
    }
    return_requested = 0;
}

int program_spawn(const char *name, const void *image, size_t image_size, uint64_t entry_offset) {
    int slot = find_slot();
    if (slot < 0) {
        return -1;
    }

    grid_program_t *program = &programs[slot];
    uint64_t *tables = memory_create_user_tables();
    if (!tables) {
        return -1;
    }

    if (memory_map_user_rx(tables, USER_CODE_VADDR, image, image_size) != 0) {
        return -1;
    }

    if (memory_map_user_rw(tables, USER_STACK_VADDR, USER_STACK_PAGES * PAGE_SIZE, 1) != 0) {
        return -1;
    }

    program->used = 1;
    program->state = PROGRAM_RUNNING;
    program->exit_code = 0;
    program->page_tables = tables;
    program->capabilities = CAP_READ_GRID | CAP_WRITE_GRID | CAP_COMMUNICATE;
    copy_name(program, name);
    derive_program_disc(program, slot);
    program->entry_point = USER_CODE_VADDR + entry_offset;

    if (program_enter(program, program->entry_point) != 0) {
        return slot + 1;
    }

    return slot + 1;
}

int program_spawn_gridprog(const char *name) {
    size_t size = (size_t)(gridprog_bin_end - gridprog_bin);
    return program_spawn(name, gridprog_bin, size, 0);
}

static int names_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

static int program_enter(grid_program_t *program, uint64_t entry) {
    return_requested = 0;
    fault_occurred = 0;
    program->preempted = 0;
    syscall_set_active_program(program);

    if (program->has_ctx) {
        enter_usermode_resume(&program->ctx, (uint64_t)program->page_tables);
    } else {
        uint64_t stack = USER_STACK_VADDR + (USER_STACK_PAGES * PAGE_SIZE);
        enter_usermode(entry, stack, (uint64_t)program->page_tables);
    }

    /* Control returns here after a preempt, a syscall exit, or a fault. */
    memory_switch_tables(memory_kernel_tables());
    syscall_set_active_program(0);

    if (fault_occurred) {
        program->state = PROGRAM_FAULT;
        program->has_ctx = 0;
        program->preempted = 0;
        return -1;
    }

    if (return_requested) {
        /* SYS_EXIT already set the state via the syscall handler. */
        program->has_ctx = 0;
        program->preempted = 0;
        return 0;
    }

    /* Timer preempt: context is saved, program stays RUNNING for its next slice. */
    return 0;
}

static int program_setup_elf(grid_program_t *program, const void *image, size_t image_size,
                             uint64_t *entry_out) {
    uint64_t *tables = memory_create_user_tables();
    uint64_t entry = 0;

    if (!tables || !entry_out) {
        return -1;
    }

    if (elf_load(tables, image, image_size, &entry) != 0) {
        return -1;
    }

    if (memory_map_user_rw(tables, USER_STACK_VADDR, USER_STACK_PAGES * PAGE_SIZE, 1) != 0) {
        return -1;
    }

    program->page_tables = tables;
    program->entry_point = entry;
    program->capabilities = CAP_READ_GRID | CAP_WRITE_GRID | CAP_COMMUNICATE;
    *entry_out = entry;
    return 0;
}

static int program_load_image(int slot, const char *name, const void *image, size_t image_size) {
    grid_program_t *program = &programs[slot];
    uint64_t entry = 0;

    if (program_setup_elf(program, image, image_size, &entry) != 0) {
        return -1;
    }

    program->used = 1;
    program->state = PROGRAM_IDLE;
    program->exit_code = 0;
    program->has_ctx = 0;
    program->preempted = 0;
    copy_name(program, name);
    derive_program_disc(program, slot);
    return slot + 1;
}

int program_run(int id) {
    if (id < 1 || id > PROGRAM_SLOTS || !programs[id - 1].used) {
        return -1;
    }

    grid_program_t *program = &programs[id - 1];
    if (program->state == PROGRAM_FAULT || program->state == PROGRAM_EXITED) {
        return -1;
    }

    program->state = PROGRAM_RUNNING;
    return program_enter(program, program->entry_point);
}

int program_load_named(const char *name) {
    static uint8_t image[PROGRAM_IMAGE_MAX];
    int slot;
    size_t len = 0;

    if (!name) {
        return -1;
    }

    slot = find_slot();
    if (slot < 0) {
        return -1;
    }

    if (gfs_present()) {
        char path[64];
        size_t pos = 0;

        path[pos++] = '/';
        path[pos++] = 'p';
        path[pos++] = 'r';
        path[pos++] = 'o';
        path[pos++] = 'g';
        path[pos++] = 'r';
        path[pos++] = 'a';
        path[pos++] = 'm';
        path[pos++] = 's';
        path[pos++] = '/';
        for (size_t i = 0; name[i] && pos + 1 < sizeof(path); ++i) {
            path[pos++] = name[i];
        }
        path[pos] = '\0';

        if (gfs_read_file(path, image, sizeof(image), &len) == 0) {
            return program_load_image(slot, name, image, len);
        }
    }

    if (names_equal(name, "gridprog")) {
        len = (size_t)(gridprog_bin_end - gridprog_bin);
        return program_load_image(slot, name, gridprog_bin, len);
    }
    if (names_equal(name, "discinfo")) {
        len = (size_t)(discinfo_bin_end - discinfo_bin);
        return program_load_image(slot, name, discinfo_bin, len);
    }
    if (names_equal(name, "gridsh")) {
        len = (size_t)(gridsh_bin_end - gridsh_bin);
        return program_load_image(slot, name, gridsh_bin, len);
    }
    if (names_equal(name, "lightcycle")) {
        len = (size_t)(lightcycle_bin_end - lightcycle_bin);
        return program_load_image(slot, name, lightcycle_bin, len);
    }
    if (names_equal(name, "gridloop")) {
        len = (size_t)(gridloop_bin_end - gridloop_bin);
        return program_load_image(slot, name, gridloop_bin, len);
    }

    return -1;
}

int program_run_background(const char *name) {
    int id = program_load_named(name);
    if (id < 0) {
        return -1;
    }
    if (sched_launch(id) != 0) {
        programs[id - 1].used = 0;
        return -1;
    }
    return id;
}

int program_spawn_elf(const char *name, const void *image, size_t image_size) {
    int slot = find_slot();
    int id;

    if (slot < 0) {
        return -1;
    }

    id = program_load_image(slot, name, image, image_size);
    if (id < 0) {
        return -1;
    }

    programs[slot].state = PROGRAM_RUNNING;
    if (program_enter(&programs[slot], programs[slot].entry_point) != 0) {
        return id;
    }

    return id;
}

int program_spawn_from_disk(const char *name) {
    static uint8_t image[PROGRAM_IMAGE_MAX];
    char path[64];
    size_t len = 0;
    size_t pos = 0;

    if (!name || !gfs_present()) {
        return -1;
    }

    path[pos++] = '/';
    path[pos++] = 'p';
    path[pos++] = 'r';
    path[pos++] = 'o';
    path[pos++] = 'g';
    path[pos++] = 'r';
    path[pos++] = 'a';
    path[pos++] = 'm';
    path[pos++] = 's';
    path[pos++] = '/';

    while (name[len] && pos + 1 < sizeof(path)) {
        path[pos++] = name[len++];
    }
    path[pos] = '\0';

    if (gfs_read_file(path, image, sizeof(image), &len) != 0) {
        return -1;
    }

    return program_spawn_elf(name, image, len);
}

int program_spawn_named(const char *name) {
    int id;

    if (!name) {
        return -1;
    }

    id = program_spawn_from_disk(name);
    if (id >= 0) {
        return id;
    }

    if (names_equal(name, "gridprog")) {
        return program_spawn_gridprog(name);
    }
    if (names_equal(name, "discinfo")) {
        size_t size = (size_t)(discinfo_bin_end - discinfo_bin);
        return program_spawn(name, discinfo_bin, size, 0);
    }
    if (names_equal(name, "gridsh")) {
        size_t size = (size_t)(gridsh_bin_end - gridsh_bin);
        return program_spawn(name, gridsh_bin, size, 0);
    }
    if (names_equal(name, "lightcycle")) {
        size_t size = (size_t)(lightcycle_bin_end - lightcycle_bin);
        return program_spawn(name, lightcycle_bin, size, 0);
    }
    if (names_equal(name, "gridloop")) {
        size_t size = (size_t)(gridloop_bin_end - gridloop_bin);
        return program_spawn(name, gridloop_bin, size, 0);
    }
    return -1;
}

void program_print_catalog(void) {
    console_write_line("Available ring-3 programs (loaded from /programs on disk):");
    console_set_color(GRID_COL_OK);
    console_write_line("  gridsh       interactive Flynn shell");
    console_write_line("  discinfo     identity disc record");
    console_write_line("  gridprog     minimal sandbox demo");
    console_write_line("  gridloop     long-running preempt demo");
    console_write_line("  lightcycle   light cycle arena v2 (WASD)");
    console_set_color(GRID_COL_DEFAULT);
    if (gfs_present()) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  Disk archive: ls /programs");
        console_set_color(GRID_COL_DEFAULT);
    }
}

void program_print_list(void) {
    int found = 0;

    console_write_line("Sandboxed programs:");
    for (int i = 0; i < PROGRAM_SLOTS; ++i) {
        const char *state;

        if (!programs[i].used) {
            continue;
        }

        switch (programs[i].state) {
        case PROGRAM_IDLE:
            state = "idle";
            break;
        case PROGRAM_RUNNING:
            state = "running";
            break;
        case PROGRAM_EXITED:
            state = "exited";
            break;
        case PROGRAM_FAULT:
            state = "fault";
            break;
        default:
            state = "unknown";
            break;
        }

        found = 1;
        console_write("  [");
        console_write_char((char)('0' + i + 1));
        console_write("] ");
        console_set_color(GRID_COL_OK);
        console_write(programs[i].name);
        console_set_color(GRID_COL_DEFAULT);
        console_write("  ");
        console_write(state);
        if (programs[i].has_ctx) {
            console_write("  resumed");
        }
        console_write("  disc=");
        char disc[33];
        program_format_disc(i + 1, disc, sizeof(disc));
        console_write_line(disc);
    }

    if (!found) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (none — use 'spawn gridsh')");
        console_set_color(GRID_COL_DEFAULT);
    }
}

void program_release(int id) {
    if (id < 1 || id > PROGRAM_SLOTS) {
        return;
    }

    grid_program_t *program = &programs[id - 1];
    if (!program->used) {
        return;
    }

    program->used = 0;
    program->state = PROGRAM_IDLE;
    program->exit_code = 0;
    program->page_tables = 0;
    program->has_ctx = 0;
    program->preempted = 0;
}
