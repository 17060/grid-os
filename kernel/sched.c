#include "program.h"
#include "sched.h"
#include "timer.h"
#include "console.h"
#include "grid.h"

#define SCHED_QUANTUM 25u
#define SCHED_JOBS_MAX 4

typedef struct {
    int program_id;
    int active;
    uint32_t slice_start;
} sched_job_t;

static sched_job_t jobs[SCHED_JOBS_MAX];
static int job_count = 0;
static int preempt_armed = 0;
static volatile int quantum_expired = 0;

void sched_init(void) {
    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        jobs[i].program_id = 0;
        jobs[i].active = 0;
        jobs[i].slice_start = 0;
    }
    job_count = 0;
    preempt_armed = 0;
    quantum_expired = 0;
}

int sched_launch(int program_id) {
    if (program_id <= 0) {
        return -1;
    }

    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        if (jobs[i].active && jobs[i].program_id == program_id) {
            return 0;
        }
    }

    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        if (!jobs[i].active) {
            jobs[i].program_id = program_id;
            jobs[i].active = 1;
            jobs[i].slice_start = timer_ticks();
            job_count++;
            preempt_armed = 1;
            return 0;
        }
    }

    return -1;
}

int sched_job_count(void) {
    return job_count;
}

void sched_kill_all(void) {
    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        if (jobs[i].active) {
            program_release(jobs[i].program_id);
            jobs[i].active = 0;
            jobs[i].program_id = 0;
        }
    }
    job_count = 0;
    preempt_armed = 0;
    quantum_expired = 0;
}

int sched_kill(int program_id) {
    int found = 0;

    if (program_id <= 0) {
        return -1;
    }

    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        if (jobs[i].active && jobs[i].program_id == program_id) {
            jobs[i].active = 0;
            jobs[i].program_id = 0;
            job_count--;
            found = 1;
        }
    }

    if (!found) {
        return -1;
    }

    program_release(program_id);
    if (job_count <= 0) {
        job_count = 0;
        preempt_armed = 0;
        quantum_expired = 0;
    }
    return 0;
}

int sched_detach(int program_id) {
    int found = 0;

    if (program_id <= 0) {
        return -1;
    }

    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        if (jobs[i].active && jobs[i].program_id == program_id) {
            jobs[i].active = 0;
            jobs[i].program_id = 0;
            job_count--;
            found = 1;
        }
    }

    if (!found) {
        return -1;
    }

    if (job_count <= 0) {
        job_count = 0;
        preempt_armed = 0;
        quantum_expired = 0;
    }

    return 0;
}

int sched_should_preempt(void) {
    if (!preempt_armed || job_count == 0) {
        return 0;
    }
    if (quantum_expired) {
        quantum_expired = 0;
        return 1;
    }
    return 0;
}

static const char *state_label(program_state_t state) {
    switch (state) {
    case PROGRAM_IDLE:
        return "idle";
    case PROGRAM_RUNNING:
        return "running";
    case PROGRAM_EXITED:
        return "exited";
    case PROGRAM_FAULT:
        return "fault";
    default:
        return "unknown";
    }
}

void sched_print_jobs(void) {
    int found = 0;

    console_write_line("Background jobs:");
    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        const grid_program_t *program;

        if (!jobs[i].active) {
            continue;
        }

        program = program_get(jobs[i].program_id);
        found = 1;
        console_write("  [");
        console_write_char((char)('0' + jobs[i].program_id));
        console_write("] ");
        if (program) {
            console_set_color(GRID_COL_OK);
            console_write(program->name);
            console_set_color(GRID_COL_DEFAULT);
            console_write("  ");
            console_write_line(state_label(program->state));
        } else {
            console_write_line("(missing program)");
        }
    }

    if (!found) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (none — use 'spawn bg gridprog')");
        console_set_color(GRID_COL_DEFAULT);
    }
}

void sched_on_timer(void) {
    if (!preempt_armed || job_count == 0) {
        return;
    }

    uint32_t now = timer_ticks();
    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        if (!jobs[i].active) {
            continue;
        }
        if (now - jobs[i].slice_start >= SCHED_QUANTUM) {
            quantum_expired = 1;
            return;
        }
    }
}

void sched_run_pending(void) {
    /* Preemption is now driven directly by the timer ISR (isr_timer),
       which saves user context and returns to the kernel. Nothing to do here. */
}

void sched_service(void) {
    for (int i = 0; i < SCHED_JOBS_MAX; ++i) {
        const grid_program_t *program;

        if (!jobs[i].active) {
            continue;
        }

        program = program_get(jobs[i].program_id);
        if (!program) {
            jobs[i].active = 0;
            job_count--;
            continue;
        }

        if (program->state == PROGRAM_EXITED || program->state == PROGRAM_FAULT) {
            jobs[i].active = 0;
            job_count--;
            continue;
        }

        jobs[i].slice_start = timer_ticks();
        quantum_expired = 0;
        program_run(jobs[i].program_id);
        return;
    }

    if (job_count == 0) {
        preempt_armed = 0;
    }
}
