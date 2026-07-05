#ifndef SECURITY_H
#define SECURITY_H

#include <stddef.h>
#include <stdint.h>

#define GRID_DISC_BYTES 16
#define GRID_DISC_HEX   (GRID_DISC_BYTES * 2 + 1)

#define CAP_READ_GRID    (1u << 0)
#define CAP_WRITE_GRID   (1u << 1)
#define CAP_SPAWN        (1u << 2)
#define CAP_COMMUNICATE  (1u << 3)
#define CAP_ADMIN        (1u << 4)
#define CAP_ISO_RESEARCH (1u << 5)
#define CAP_STORAGE      (1u << 6)

typedef enum {
    ENTITY_USER = 0,
    ENTITY_PROGRAM = 1
} grid_entity_t;

typedef struct {
    uint8_t disc[GRID_DISC_BYTES];
    grid_entity_t entity;
    uint32_t capabilities;
    uint32_t cycle_count;
    int authenticated;
} grid_identity_t;

void security_init(void);
const grid_identity_t *security_current_identity(void);
int security_has_capability(uint32_t cap);
int security_require_capability(uint32_t cap, const char *action);
void security_format_disc(char *out, size_t out_len);
void security_restore_state(const uint8_t disc[GRID_DISC_BYTES], uint32_t cycles);
void security_tick(void);
uint32_t security_cycles(void);
const char *security_entity_name(void);

#endif
