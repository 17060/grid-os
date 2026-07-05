#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(void);
uint32_t timer_ticks(void);
int timer_autopilot_enabled(void);
void timer_set_autopilot(int enabled);

#endif
