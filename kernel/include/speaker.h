#ifndef SPEAKER_H
#define SPEAKER_H

#include <stdint.h>

void speaker_init(void);
void speaker_beep(uint32_t freq_hz, uint32_t ms);
void speaker_note(int note, uint32_t ms);

#endif
