#include "speaker.h"

#include "timer.h"

#define PIT_CH2 0x42
#define PIT_CMD 0x43
#define PCSPKR  0x61

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void speaker_init(void) {
}

void speaker_beep(uint32_t freq_hz, uint32_t ms) {
    if (freq_hz < 20) {
        freq_hz = 20;
    }
    if (freq_hz > 20000) {
        freq_hz = 20000;
    }
    if (ms == 0) {
        return;
    }

    uint32_t div = 1193182u / freq_hz;
    if (div > 65535u) {
        div = 65535u;
    }

    outb(PIT_CMD, 0xB6);
    outb(PIT_CH2, (uint8_t)(div & 0xFF));
    outb(PIT_CH2, (uint8_t)((div >> 8) & 0xFF));

    uint8_t gate = inb(PCSPKR);
    outb(PCSPKR, (uint8_t)(gate | 3));

    uint32_t start = timer_ticks();
    while (timer_ticks() - start < ms) {
        /* spin */
    }

    outb(PCSPKR, (uint8_t)(gate & (uint8_t)~3));
}

void speaker_note(int note, uint32_t ms) {
    static const uint16_t freqs[] = {
        262, 294, 330, 349, 392, 440, 494,
        523, 587, 659, 698, 784, 880, 988
    };
    if (note < 0) {
        note = 0;
    }
    if (note > 13) {
        note = 13;
    }
    speaker_beep(freqs[note], ms);
}
