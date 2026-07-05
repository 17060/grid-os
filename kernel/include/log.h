#ifndef LOG_H
#define LOG_H

#include <stddef.h>

#define LOG_ENTRIES 32

void log_init(void);
void log_event(const char *message);
void log_print_all(void);
void log_print_tail(int count);

#endif
