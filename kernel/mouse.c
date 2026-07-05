#include "mouse.h"

#include <stdint.h>

static int present = 0;
static int packet_index = 0;
static uint8_t packet[3];
static int mx = 40;
static int my = 12;
static int moved = 0;
static int clicked = 0;
static uint8_t buttons = 0;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int wait_write(void) {
    for (int i = 0; i < 100000; ++i) {
        if ((inb(0x64) & 0x02) == 0) {
            return 0;
        }
    }
    return -1;
}

static int wait_read(void) {
    for (int i = 0; i < 100000; ++i) {
        if (inb(0x64) & 0x01) {
            return 0;
        }
    }
    return -1;
}

static void write_command(uint8_t value) {
    if (wait_write() != 0) {
        return;
    }
    outb(0x64, value);
}

static void write_data(uint8_t value) {
    if (wait_write() != 0) {
        return;
    }
    outb(0x60, value);
}

static int read_data(void) {
    if (wait_read() != 0) {
        return -1;
    }
    return (int)inb(0x60);
}

static void flush_device(void) {
    while (inb(0x64) & 0x01) {
        (void)inb(0x60);
    }
}

static void process_packet(void) {
    int dx = (int)packet[1];
    int dy = (int)packet[2];

    if (packet[0] & 0x10) {
        dx -= 256;
    }
    if (packet[0] & 0x20) {
        dy -= 256;
    }

    mx += dx;
    my -= dy;

    if (mx < 0) {
        mx = 0;
    }
    if (my < 0) {
        my = 0;
    }
    if (mx >= 80) {
        mx = 79;
    }
    if (my >= 25) {
        my = 24;
    }

    buttons = (uint8_t)(packet[0] & 0x07u);
    moved = 1;

    if (buttons & 0x01u) {
        clicked = 1;
    }
}

void mouse_init(void) {
    present = 0;
    packet_index = 0;
    moved = 0;
    clicked = 0;
    buttons = 0;
    mx = 40;
    my = 12;

    flush_device();

    write_command(0xA8);
    write_command(0x20);
    if (read_data() < 0) {
        return;
    }
    write_data(0x20);

    write_command(0xD4);
    write_data(0xF4);
    if (read_data() < 0) {
        return;
    }

    (void)read_data();
    present = 1;
}

int mouse_present(void) {
    return present;
}

void mouse_poll(void) {
    int value;

    if (!present) {
        return;
    }

    while (inb(0x64) & 0x01) {
        value = read_data();
        if (value < 0) {
            return;
        }

        if (packet_index == 0 && !(value & 0x08)) {
            continue;
        }

        packet[packet_index++] = (uint8_t)value;
        if (packet_index >= 3) {
            packet_index = 0;
            process_packet();
        }
    }
}

int mouse_moved(void) {
    return moved;
}

int mouse_clicked(void) {
    return clicked;
}

uint8_t mouse_buttons(void) {
    return buttons;
}

int mouse_x(void) {
    return mx;
}

int mouse_y(void) {
    return my;
}

void mouse_consume_click(void) {
    clicked = 0;
    moved = 0;
}
