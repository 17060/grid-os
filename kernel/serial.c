#include "serial.h"

#include <stddef.h>
#include <stdint.h>

static int online = 0;

/* QEMU -serial stdio loops TX bytes back to RX. Track recent TX so we can
 * discard loopback echoes before they reach the shell line editor. */
#define SER_ECHO_RING 8192u
static char ser_tx_ring[SER_ECHO_RING];
static unsigned ser_tx_w = 0;
static unsigned ser_tx_r = 0;

static void ser_tx_record(char byte) {
    ser_tx_ring[ser_tx_w % SER_ECHO_RING] = byte;
    ser_tx_w++;
    if (ser_tx_w - ser_tx_r > SER_ECHO_RING) {
        ser_tx_r = ser_tx_w - SER_ECHO_RING;
    }
}

static int ser_tx_consume_echo(char byte) {
    if (ser_tx_r == ser_tx_w) {
        return 0;
    }
    if (ser_tx_ring[ser_tx_r % SER_ECHO_RING] != byte) {
        return 0;
    }
    ser_tx_r++;
    return 1;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x01);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
    online = 1;
}

int serial_is_online(void) {
    return online;
}

int serial_can_read(void) {
    return (inb(COM1_PORT + 5) & 0x01) != 0;
}

int serial_read_byte(void) {
    if (!serial_can_read()) {
        return -1;
    }
    char byte = (char)inb(COM1_PORT);
    if (byte == '\0') {
        return -1;
    }
    if (ser_tx_consume_echo(byte)) {
        return -1;
    }
    return (int)(unsigned char)byte;
}

/* Keep this out-of-line: at -O2 GCC inlines serial_write_byte into
 * serial_write and miscompiles the loop so it walks past the terminating NUL
 * and dumps the entire .rodata section to COM1 (megabytes of garbage). */
__attribute__((noinline))
void serial_write_byte(char byte) {
    while ((inb(COM1_PORT + 5) & 0x20) == 0) {
    }
    outb(COM1_PORT, (uint8_t)byte);
    ser_tx_record(byte);
}

void serial_write(const char *text) {
    const char *p;
    if (!text) {
        return;
    }
    for (p = text; *p; ++p) {
        serial_write_byte(*p);
    }
}

size_t serial_read_line(char *buffer, size_t capacity, uint32_t spin_limit) {
    size_t length = 0;
    uint32_t spins = 0;

    if (capacity == 0) {
        return 0;
    }

    buffer[0] = '\0';

    while (spins < spin_limit) {
        int raw = serial_read_byte();
        if (raw >= 0) {
            char c = (char)raw;
            if (c == '\r') {
                spins = 0;
                continue;
            }
            if (c == '\n') {
                buffer[length] = '\0';
                return length;
            }
            if (length + 1 < capacity) {
                buffer[length++] = c;
                buffer[length] = '\0';
            }
            spins = 0;
            continue;
        }
        spins++;
    }

    buffer[length] = '\0';
    return length;
}
