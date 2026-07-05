#include "console.h"
#include "log.h"
#include "security.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t cycle;
    char message[64];
} log_entry_t;

static log_entry_t entries[LOG_ENTRIES];
static int head = 0;
static int count = 0;

void log_init(void) {
    head = 0;
    count = 0;
}

void log_event(const char *message) {
    log_entry_t *entry = &entries[head];

    entry->cycle = security_cycles();
    size_t i = 0;
    if (message) {
        while (message[i] && i + 1 < sizeof(entry->message)) {
            entry->message[i] = message[i];
            i++;
        }
    }
    entry->message[i] = '\0';

    head = (head + 1) % LOG_ENTRIES;
    if (count < LOG_ENTRIES) {
        count++;
    }
}

static void print_uint(uint32_t value) {
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

void log_print_all(void) {
    log_print_tail(LOG_ENTRIES);
}

void log_print_tail(int tail) {
    int start = 0;
    int show = count;

    if (tail > 0 && tail < show) {
        start = count - tail;
        show = tail;
    }

    console_write_line("Grid audit log:");
    if (count == 0) {
        console_set_color(GRID_COL_DIM);
        console_write_line("  (empty — events appear as you use the Grid)");
        console_set_color(GRID_COL_DEFAULT);
        return;
    }

    for (int i = start; i < count; ++i) {
        int index = (head - count + i + LOG_ENTRIES) % LOG_ENTRIES;
        console_write("  [");
        print_uint(entries[index].cycle);
        console_write("] ");
        console_write_line(entries[index].message);
    }
}
