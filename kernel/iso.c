#include "console.h"
#include "iso.h"
#include "security.h"

#include <stddef.h>
#include <stdint.h>

static iso_entity_t zone[ISO_ZONE_SLOTS];
static uint32_t rng_state = 0x49534F00; /* ISO\0 */

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static uint32_t popcount8(uint8_t value) {
    uint32_t count = 0;
    while (value) {
        count += value & 1u;
        value >>= 1;
    }
    return count;
}

static uint32_t genome_fitness(const uint8_t genome[ISO_GENOME_SIZE], uint32_t generation) {
    uint32_t score = generation;
    for (size_t i = 0; i < ISO_GENOME_SIZE; ++i) {
        score += popcount8(genome[i]);
    }
    return score;
}

static uint8_t genome_checksum(const uint8_t genome[ISO_GENOME_SIZE]) {
    uint8_t sum = 0x5A;
    for (size_t i = 0; i < ISO_GENOME_SIZE; ++i) {
        sum ^= genome[i];
    }
    return sum;
}

static int genome_valid(const uint8_t genome[ISO_GENOME_SIZE]) {
    return genome[ISO_GENOME_SIZE - 1] == genome_checksum(genome);
}

static void genome_seal(uint8_t genome[ISO_GENOME_SIZE]) {
    genome[ISO_GENOME_SIZE - 1] = genome_checksum(genome);
}

static void derive_iso_disc(iso_entity_t *iso, int slot) {
    const grid_identity_t *parent = security_current_identity();
    uint32_t state = 0x49534F7A ^ (uint32_t)(slot + 1) ^ (uint32_t)iso->generation;

    for (size_t i = 0; i < 16; i += 4) {
        uint32_t block = xorshift32(&state);
        block ^= *(const uint32_t *)(parent->disc + i);
        iso->disc[i] = (uint8_t)(block & 0xFF);
        iso->disc[i + 1] = (uint8_t)((block >> 8) & 0xFF);
        iso->disc[i + 2] = (uint8_t)((block >> 16) & 0xFF);
        iso->disc[i + 3] = (uint8_t)((block >> 24) & 0xFF);
    }
}

static void seed_genome(uint8_t genome[ISO_GENOME_SIZE]) {
    for (size_t i = 0; i + 1 < ISO_GENOME_SIZE; ++i) {
        genome[i] = (uint8_t)(xorshift32(&rng_state) & 0xFF);
    }
    genome_seal(genome);
}

static int find_slot(void) {
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        if (!zone[i].used) {
            return i;
        }
    }
    return -1;
}

static int validate_id(int id) {
    if (id < 1 || id > ISO_ZONE_SLOTS) {
        return 0;
    }
    return zone[id - 1].used;
}

static iso_entity_t *entity_at(int id) {
    if (!validate_id(id)) {
        return 0;
    }
    return &zone[id - 1];
}

static void copy_name(iso_entity_t *iso, const char *name) {
    size_t i = 0;
    if (name) {
        while (name[i] && i + 1 < ISO_NAME_MAX) {
            iso->name[i] = name[i];
            i++;
        }
    }
    if (i == 0) {
        iso->name[0] = 'i';
        iso->name[1] = 's';
        iso->name[2] = 'o';
        iso->name[3] = '-';
        iso->name[4] = (char)('0' + (iso - zone + 1));
        iso->name[5] = '\0';
        return;
    }
    iso->name[i] = '\0';
}

void iso_zone_init(void) {
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        zone[i].used = 0;
    }
}

int iso_zone_count(void) {
    int count = 0;
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        if (zone[i].used) {
            count++;
        }
    }
    return count;
}

int iso_zone_active_count(void) {
    int count = 0;
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        if (zone[i].used && zone[i].state == ISO_STATE_ACTIVE) {
            count++;
        }
    }
    return count;
}

int iso_spawn(const char *name) {
    int slot = find_slot();
    if (slot < 0) {
        return -1;
    }

    iso_entity_t *iso = &zone[slot];
    iso->used = 1;
    iso->generation = 0;
    iso->state = ISO_STATE_ACTIVE;
    copy_name(iso, name);
    seed_genome(iso->genome);
    derive_iso_disc(iso, slot);
    iso->fitness = genome_fitness(iso->genome, iso->generation);
    return slot + 1;
}

int iso_evolve(int id) {
    iso_entity_t *iso = entity_at(id);
    if (!iso) {
        return -1;
    }
    if (iso->state != ISO_STATE_ACTIVE) {
        return -2;
    }

    uint8_t backup[ISO_GENOME_SIZE];
    for (size_t i = 0; i < ISO_GENOME_SIZE; ++i) {
        backup[i] = iso->genome[i];
    }

    size_t index = (size_t)(xorshift32(&rng_state) % (ISO_GENOME_SIZE - 1));
    iso->genome[index] ^= (uint8_t)(1u << (xorshift32(&rng_state) & 7u));
    genome_seal(iso->genome);

    if (!genome_valid(iso->genome)) {
        for (size_t i = 0; i < ISO_GENOME_SIZE; ++i) {
            iso->genome[i] = backup[i];
        }
        return -3;
    }

    iso->generation++;
    derive_iso_disc(iso, id - 1);
    iso->fitness = genome_fitness(iso->genome, iso->generation);
    return 0;
}

int iso_quarantine(int id) {
    iso_entity_t *iso = entity_at(id);
    if (!iso) {
        return -1;
    }
    iso->state = ISO_STATE_QUARANTINED;
    return 0;
}

int iso_release(int id) {
    iso_entity_t *iso = entity_at(id);
    if (!iso) {
        return -1;
    }
    iso->state = ISO_STATE_ACTIVE;
    return 0;
}

const iso_entity_t *iso_get(int id) {
    return entity_at(id);
}

void iso_format_disc(int id, char *out, size_t out_len) {
    static const char hex[] = "0123456789ABCDEF";
    const iso_entity_t *iso = entity_at(id);
    size_t pos = 0;

    if (!iso || out_len == 0) {
        return;
    }

    for (size_t i = 0; i < 16; ++i) {
        if (pos + 2 >= out_len) {
            break;
        }
        out[pos++] = hex[(iso->disc[i] >> 4) & 0xF];
        out[pos++] = hex[iso->disc[i] & 0xF];
    }

    if (pos < out_len) {
        out[pos] = '\0';
    } else {
        out[out_len - 1] = '\0';
    }
}

void iso_format_genome(int id, char *out, size_t out_len) {
    static const char hex[] = "0123456789ABCDEF";
    const iso_entity_t *iso = entity_at(id);
    size_t pos = 0;

    if (!iso || out_len == 0) {
        return;
    }

    for (size_t i = 0; i + 1 < ISO_GENOME_SIZE; ++i) {
        if (pos + 2 >= out_len) {
            break;
        }
        out[pos++] = hex[(iso->genome[i] >> 4) & 0xF];
        out[pos++] = hex[iso->genome[i] & 0xF];
    }

    if (pos < out_len) {
        out[pos] = '\0';
    } else {
        out[out_len - 1] = '\0';
    }
}

const char *iso_state_name(iso_state_t state) {
    switch (state) {
    case ISO_STATE_DORMANT:
        return "dormant";
    case ISO_STATE_ACTIVE:
        return "active";
    case ISO_STATE_QUARANTINED:
        return "quarantined";
    default:
        return "unknown";
    }
}

static void write_uint32(uint32_t value) {
    char buffer[16];
    size_t pos = 0;

    if (value == 0) {
        console_write("0");
        return;
    }

    while (value > 0) {
        buffer[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (pos > 0) {
        console_write_char(buffer[--pos]);
    }
}

static void write_uint32_line(uint32_t value) {
    write_uint32(value);
    console_write_char('\n');
}

void iso_print_zone(void) {
    console_set_color(GRID_COL_WARN);
    console_write_line("ISO Research Zone");
    console_set_color(GRID_COL_DEFAULT);
    console_write_line("  Policy: emergence welcome, sandbox enforced, quarantine over derez");
    console_write("  Slots:  ");
    write_uint32_line((uint32_t)ISO_ZONE_SLOTS);
    console_write("  Occupied: ");
    write_uint32_line((uint32_t)iso_zone_count());
    console_write("  Active:   ");
    write_uint32_line((uint32_t)iso_zone_active_count());
}

void iso_print_list(void) {
    int found = 0;

    console_write_line("ISO entities:");
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        if (!zone[i].used) {
            continue;
        }

        found = 1;
        console_write("  [");
        write_uint32((uint32_t)(i + 1));
        console_write("] ");
        console_set_color(GRID_COL_OK);
        console_write(zone[i].name);
        console_set_color(GRID_COL_DEFAULT);
        console_write("  state=");
        console_write(iso_state_name(zone[i].state));
        console_write("  gen=");
        write_uint32(zone[i].generation);
        console_write("  fitness=");
        write_uint32_line(zone[i].fitness);
    }

    if (!found) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (empty — use 'iso spawn' to seed the frontier)");
        console_set_color(GRID_COL_DEFAULT);
    }
}

int iso_format_list(char *out, size_t out_len) {
    if (!out || out_len == 0) {
        return 0;
    }
    size_t pos = 0;
    out[0] = '\0';
    int found = 0;

    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        if (!zone[i].used) {
            continue;
        }
        if (pos > 0 && pos + 2 < out_len) {
            out[pos++] = ',';
            out[pos++] = ' ';
        }
        if (pos + 4 < out_len) {
            out[pos++] = '#';
            out[pos++] = (char)('0' + (i + 1));
            out[pos++] = ':';
        }
        for (size_t j = 0; zone[i].name[j] && pos + 1 < out_len; ++j) {
            out[pos++] = zone[i].name[j];
        }
        found = 1;
    }
    if (!found && pos + 5 < out_len) {
        out[pos++] = 'e';
        out[pos++] = 'm';
        out[pos++] = 'p';
        out[pos++] = 't';
        out[pos++] = 'y';
    }
    out[pos] = '\0';
    return (int)pos;
}

void iso_print_inspect(int id) {
    const iso_entity_t *iso = entity_at(id);
    char buffer[96];

    if (!iso) {
        console_set_color(GRID_COL_ERROR);
        console_write_line("ISO not found.");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    console_write("ISO #");
    write_uint32_line((uint32_t)id);
    console_write("Name:      ");
    console_set_color(GRID_COL_OK);
    console_write_line(iso->name);
    console_set_color(GRID_COL_DEFAULT);

    console_write("State:     ");
    console_write_line(iso_state_name(iso->state));

    console_write("Generation:");
    write_uint32_line(iso->generation);
    console_write("Fitness:   ");
    write_uint32_line(iso->fitness);

    iso_format_disc(id, buffer, sizeof(buffer));
    console_write("Disc:      ");
    console_set_color(GRID_COL_OK);
    console_write_line(buffer);
    console_set_color(GRID_COL_DEFAULT);

    iso_format_genome(id, buffer, sizeof(buffer));
    console_write("Genome:    ");
    console_write_line(buffer);
    console_write_line("  (self-modifying payload sealed by sandbox checksum)");
}

void iso_clear_all(void) {
    iso_zone_init();
}

int iso_import_entity(int id, const char *name, const uint8_t disc[16],
                      const uint8_t genome[ISO_GENOME_SIZE], uint32_t generation,
                      uint32_t fitness, iso_state_t state) {
    if (id < 1 || id > ISO_ZONE_SLOTS) {
        return -1;
    }

    iso_entity_t *iso = &zone[id - 1];
    iso->used = 1;
    copy_name(iso, name);
    for (size_t i = 0; i < 16; ++i) {
        iso->disc[i] = disc[i];
    }
    for (size_t i = 0; i < ISO_GENOME_SIZE; ++i) {
        iso->genome[i] = genome[i];
    }
    iso->generation = generation;
    iso->fitness = fitness;
    iso->state = state;
    return 0;
}

void iso_autopilot_step(void) {
    for (int i = 0; i < ISO_ZONE_SLOTS; ++i) {
        if (!zone[i].used || zone[i].state != ISO_STATE_ACTIVE) {
            continue;
        }
        iso_evolve(i + 1);
        return;
    }
}
