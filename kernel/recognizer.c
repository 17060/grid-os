#include "recognizer.h"

#include "gfs.h"
#include "log.h"
#include "timer.h"

static int patrol = 0;
static uint32_t patrol_start = 0;
static uint32_t patrol_sectors = 0;

void recognizer_init(void) {
    patrol = 0;
    patrol_sectors = 0;
}

void recognizer_start_patrol(void) {
    patrol = 1;
    patrol_start = timer_ticks();
    log_event("Recognizer patrol started");
}

void recognizer_stop_patrol(void) {
    patrol = 0;
    log_event("Recognizer patrol stopped");
}

int recognizer_patrol_active(void) {
    return patrol;
}

void recognizer_tick(void) {
    if (!patrol) {
        return;
    }
    if ((timer_ticks() - patrol_start) % 500 == 0) {
        patrol_sectors++;
        if (gfs_present()) {
            log_event("Recognizer: sector sweep");
        }
    }
}

void recognizer_status(char *out, size_t cap) {
    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    if (!patrol) {
        size_t n = 0;
        const char *s = "Recognizer idle";
        while (*s && n + 1 < cap) {
            out[n++] = *s++;
        }
        out[n] = '\0';
        return;
    }
    size_t n = 0;
    const char *s = "Recognizer patrol sectors=";
    while (*s && n + 1 < cap) {
        out[n++] = *s++;
    }
    uint32_t v = patrol_sectors;
    char tmp[12];
    int t = 0;
    if (v == 0) {
        tmp[t++] = '0';
    } else {
        while (v > 0) {
            tmp[t++] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    while (t > 0 && n + 1 < cap) {
        out[n++] = tmp[--t];
    }
    out[n] = '\0';
}
