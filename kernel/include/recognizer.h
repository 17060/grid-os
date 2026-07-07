#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include <stddef.h>

void recognizer_init(void);
void recognizer_tick(void);
void recognizer_start_patrol(void);
void recognizer_stop_patrol(void);
int recognizer_patrol_active(void);
void recognizer_status(char *out, size_t cap);

#endif
