#ifndef ISO_H
#define ISO_H

#include <stddef.h>
#include <stdint.h>

#define ISO_ZONE_SLOTS   4
#define ISO_GENOME_SIZE  32
#define ISO_NAME_MAX     16

typedef enum {
    ISO_STATE_DORMANT = 0,
    ISO_STATE_ACTIVE = 1,
    ISO_STATE_QUARANTINED = 2
} iso_state_t;

typedef struct {
    int used;
    char name[ISO_NAME_MAX];
    uint8_t disc[16];
    uint8_t genome[ISO_GENOME_SIZE];
    uint32_t generation;
    uint32_t fitness;
    iso_state_t state;
} iso_entity_t;

void iso_zone_init(void);
int iso_zone_count(void);
int iso_zone_active_count(void);

int iso_spawn(const char *name);
int iso_evolve(int id);
int iso_quarantine(int id);
int iso_release(int id);
const iso_entity_t *iso_get(int id);

void iso_format_disc(int id, char *out, size_t out_len);
void iso_format_genome(int id, char *out, size_t out_len);
const char *iso_state_name(iso_state_t state);

void iso_print_list(void);
void iso_print_inspect(int id);
void iso_print_zone(void);

int iso_import_entity(int id, const char *name, const uint8_t disc[16],
                      const uint8_t genome[ISO_GENOME_SIZE], uint32_t generation,
                      uint32_t fitness, iso_state_t state);
void iso_clear_all(void);
void iso_autopilot_step(void);

#endif
