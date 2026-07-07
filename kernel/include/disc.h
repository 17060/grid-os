#ifndef DISC_H
#define DISC_H

#include <stddef.h>

void disc_init(void);
void disc_on_program_run(const char *name);
void disc_on_basic_run(void);
void disc_on_module_run(const char *name);
void disc_on_duel(void);
int disc_level(void);
int disc_xp(void);
void disc_format_status(char *out, size_t cap);
const char *disc_entity(void);

#endif
