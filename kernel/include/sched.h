#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

void sched_init(void);
int sched_launch(int program_id);
void sched_on_timer(void);
void sched_run_pending(void);
void sched_service(void);
int sched_job_count(void);
void sched_print_jobs(void);
int sched_kill(int program_id);
int sched_detach(int program_id);
void sched_kill_all(void);

#endif
