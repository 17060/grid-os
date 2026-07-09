/*
 * Host unit test for kernel/sched.c.
 *
 * Locks in two scheduler properties that are otherwise only observable by
 * booting the OS and eyeballing serial output:
 *   1. Round-robin dispatch — sched_service() must cycle through active jobs
 *      instead of re-picking the lowest slot every time (no starvation).
 *   2. Quantum fairness — the preempt decision must be based on the CURRENTLY
 *      running job's slice, not on an idle job's stale slice_start.
 *
 * Compiles sched.c against lightweight stubs for program/timer/console.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "program.h"
#include "sched.h"
#include "timer.h"
#include "console.h"

/* Non-static in sched.c; declared here (normally used only by the timer ISR). */
extern int sched_should_preempt(void);

/* ---- mock timer ---- */
static uint32_t g_ticks = 0;
uint32_t timer_ticks(void) { return g_ticks; }

/* ---- mock program table (indexed by 1-based id) ---- */
#define MAX_ID 8
static grid_program_t g_progs[MAX_ID + 1];
static int g_present[MAX_ID + 1];

const grid_program_t *program_get(int id) {
    if (id < 1 || id > MAX_ID || !g_present[id]) return NULL;
    return &g_progs[id];
}
void program_release(int id) { (void)id; }

/* ---- program_run: records dispatch order; in fairness mode also drives one
 * timer tick while the job is "running" and captures the preempt decision. ---- */
static int g_seq[256];
static int g_seq_n = 0;
static int g_fairness_mode = 0;
static uint32_t g_advance_in_run = 0;
static int g_preempt_seen = -1;

int program_run(int id) {
    if (g_seq_n < (int)(sizeof(g_seq) / sizeof(g_seq[0]))) {
        g_seq[g_seq_n++] = id;
    }
    if (g_fairness_mode) {
        g_ticks += g_advance_in_run;   /* time passes while the job runs */
        sched_on_timer();              /* the periodic tick's scheduler hook */
        g_preempt_seen = sched_should_preempt();
    }
    return 0;
}

/* ---- console stubs ---- */
void console_set_color(uint8_t c) { (void)c; }
void console_write(const char *s) { (void)s; }
void console_write_line(const char *s) { (void)s; }
void console_write_char(char c) { (void)c; }

static void add_prog(int id) {
    g_present[id] = 1;
    g_progs[id].used = 1;
    g_progs[id].state = PROGRAM_RUNNING;
}

int main(void) {
    /* ---- Test 1: round-robin, no starvation ---- */
    g_ticks = 0; g_seq_n = 0; g_fairness_mode = 0;
    memset(g_present, 0, sizeof(g_present));
    sched_init();
    add_prog(1); add_prog(2); add_prog(3);
    if (sched_launch(1) != 0 || sched_launch(2) != 0 || sched_launch(3) != 0) {
        printf("sched host test FAILED: launch\n"); return 1;
    }
    if (sched_job_count() != 3) {
        printf("sched host test FAILED: job_count %d != 3\n", sched_job_count()); return 1;
    }
    for (int k = 0; k < 9; ++k) { sched_service(); g_ticks += 1; }
    const int expect[9] = {1, 2, 3, 1, 2, 3, 1, 2, 3};
    for (int k = 0; k < 9; ++k) {
        if (g_seq[k] != expect[k]) {
            printf("sched host test FAILED: dispatch[%d]=%d want %d "
                   "(old flat scan re-picks slot 0 -> starvation)\n",
                   k, g_seq[k], expect[k]);
            return 1;
        }
    }
    int cnt[4] = {0, 0, 0, 0};
    for (int k = 0; k < 9; ++k) cnt[g_seq[k]]++;
    if (cnt[1] != 3 || cnt[2] != 3 || cnt[3] != 3) {
        printf("sched host test FAILED: unequal shares %d/%d/%d\n", cnt[1], cnt[2], cnt[3]);
        return 1;
    }

    /* ---- Test 2: quantum fairness uses the running job's slice ---- */
    g_ticks = 0; g_seq_n = 0; g_fairness_mode = 1;
    memset(g_present, 0, sizeof(g_present));
    sched_init();
    add_prog(1); add_prog(2);
    sched_launch(1); sched_launch(2);      /* both slice_start captured at tick 0 */
    g_ticks = 100;                         /* make both launch-time slices stale */

    /* Dispatch job 1: sched_service refreshes its slice to 100. No time passes
     * inside the run. The running job's own quantum has NOT elapsed, so it must
     * NOT be preempted -- even though the idle job 2 still shows slice_start 0.
     * The old (buggy) code checked every job and would preempt here. */
    g_advance_in_run = 0; g_preempt_seen = -1;
    sched_service();
    if (g_preempt_seen != 0) {
        printf("sched host test FAILED: running job preempted on an idle job's "
               "stale slice (fairness bug)\n");
        return 1;
    }

    /* Dispatch job 2: let time advance past its quantum while it runs; now the
     * RUNNING job's own slice is expired, so it should preempt. */
    g_advance_in_run = 30 /* > SCHED_QUANTUM (25) */; g_preempt_seen = -1;
    sched_service();
    if (g_preempt_seen != 1) {
        printf("sched host test FAILED: running job not preempted after its own "
               "quantum expired\n");
        return 1;
    }

    /* ---- Test 3: kill removes a job from rotation ---- */
    g_ticks = 0; g_seq_n = 0; g_fairness_mode = 0;
    memset(g_present, 0, sizeof(g_present));
    sched_init();
    add_prog(1); add_prog(2);
    sched_launch(1); sched_launch(2);
    sched_kill(1);
    if (sched_job_count() != 1) {
        printf("sched host test FAILED: job_count after kill %d != 1\n", sched_job_count());
        return 1;
    }
    for (int k = 0; k < 4; ++k) sched_service();
    for (int k = 0; k < g_seq_n; ++k) {
        if (g_seq[k] != 2) {
            printf("sched host test FAILED: killed job 1 still dispatched\n");
            return 1;
        }
    }

    printf("sched host tests OK\n");
    return 0;
}
