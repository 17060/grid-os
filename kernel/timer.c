#include "iso.h"
#include "recognizer.h"
#include "sched.h"
#include "security.h"
#include "timer.h"

#include <stdint.h>

static volatile uint32_t ticks = 0;
static int autopilot = 0;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void pic_init(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

static void pit_init(void) {
    uint32_t divisor = 1193182 / 100;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void timer_init(void) {
    pic_init();
    pit_init();
    ticks = 0;
    autopilot = 0;
}

uint32_t timer_ticks(void) {
    return ticks;
}

int timer_autopilot_enabled(void) {
    return autopilot;
}

void timer_set_autopilot(int enabled) {
    autopilot = enabled ? 1 : 0;
}

void timer_on_irq(void) {
    outb(0x20, 0x20);
    ticks++;

    if ((ticks % 100u) == 0u) {
        security_tick();
    }

    sched_on_timer();

    if (autopilot && (ticks % 100u) == 0u) {
        iso_autopilot_step();
    }

    if ((ticks % 50u) == 0u) {
        recognizer_tick();
    }
}
