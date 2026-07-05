#include "console.h"
#include "log.h"
#include "security.h"

#include <stddef.h>
#include <stdint.h>

static grid_identity_t current_identity;

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void derive_identity_disc(uint8_t out[GRID_DISC_BYTES]) {
    uint32_t state = 0x464C594E; /* FLYN */

    for (size_t i = 0; i < GRID_DISC_BYTES; i += 4) {
        uint32_t block = xorshift32(&state);
        out[i] = (uint8_t)(block & 0xFF);
        if (i + 1 < GRID_DISC_BYTES) {
            out[i + 1] = (uint8_t)((block >> 8) & 0xFF);
        }
        if (i + 2 < GRID_DISC_BYTES) {
            out[i + 2] = (uint8_t)((block >> 16) & 0xFF);
        }
        if (i + 3 < GRID_DISC_BYTES) {
            out[i + 3] = (uint8_t)((block >> 24) & 0xFF);
        }
    }
}

void security_init(void) {
    derive_identity_disc(current_identity.disc);
    current_identity.entity = ENTITY_USER;
    current_identity.capabilities =
        CAP_READ_GRID | CAP_WRITE_GRID | CAP_SPAWN | CAP_COMMUNICATE |
        CAP_ISO_RESEARCH | CAP_STORAGE;
    current_identity.cycle_count = 0;
    current_identity.authenticated = 1;
}

const grid_identity_t *security_current_identity(void) {
    return &current_identity;
}

int security_has_capability(uint32_t cap) {
    if (!current_identity.authenticated) {
        return 0;
    }
    return (current_identity.capabilities & cap) == cap;
}

int security_require_capability(uint32_t cap, const char *action) {
    if (security_has_capability(cap)) {
        return 1;
    }

    console_set_color(GRID_COL_ERROR);
    console_write("ACCESS DENIED: missing capability for ");
    console_write(action);
    console_write_line("");
    console_set_color(GRID_COL_DEFAULT);
    log_event("capability denied");
    return 0;
}

void security_format_disc(char *out, size_t out_len) {
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = 0;

    for (size_t i = 0; i < GRID_DISC_BYTES; ++i) {
        if (pos + 2 >= out_len) {
            break;
        }
        out[pos++] = hex[(current_identity.disc[i] >> 4) & 0xF];
        out[pos++] = hex[current_identity.disc[i] & 0xF];
    }

    if (pos < out_len) {
        out[pos] = '\0';
    } else if (out_len > 0) {
        out[out_len - 1] = '\0';
    }
}

void security_tick(void) {
    current_identity.cycle_count++;
}

uint32_t security_cycles(void) {
    return current_identity.cycle_count;
}

const char *security_entity_name(void) {
    return current_identity.entity == ENTITY_USER ? "User" : "Program";
}

void security_restore_state(const uint8_t disc[GRID_DISC_BYTES], uint32_t cycles) {
    for (size_t i = 0; i < GRID_DISC_BYTES; ++i) {
        current_identity.disc[i] = disc[i];
    }
    current_identity.cycle_count = cycles;
    current_identity.authenticated = 1;
}
