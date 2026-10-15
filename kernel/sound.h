#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void sound_init(void);
void play_sound(uint32_t nFreq);
void nosound(void);
void beep(uint32_t freq, uint32_t duration);

#endif // SOUND_H