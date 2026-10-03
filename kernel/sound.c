#include <stdint.h>
#include "sound.h"
#include "io.h"

void play_sound(uint32_t nFreq) {
    if (nFreq == 0) return;

    uint32_t Div = 1193180 / nFreq;
    
    // Command PIT Channel 2 (Square wave generator mode 3)
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(Div & 0xFF));        // Low byte
    outb(0x42, (uint8_t)((Div >> 8) & 0xFF)); // High byte

    // Enable speaker gate on Port 0x61 (set bits 0 and 1)
    uint8_t tmp = inb(0x61);
    if ((tmp & 3) != 3) {
        outb(0x61, tmp | 3);
    }
}

void nosound(void) {
    // Disable speaker gate by clearing bits 0 and 1 on Port 0x61
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

void beep(uint32_t freq, uint32_t duration) {
    play_sound(freq);
    for (volatile uint32_t i = 0; i < duration * 100000; i++) {
        __asm__ volatile ("nop");
    }
    nosound();
}

void sound_init(void) {
    nosound();
}