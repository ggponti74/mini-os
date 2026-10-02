// kernel/sound.c
#include "sound.h"
#include "io.h"

void play_sound(uint32_t nFreq) {
    if (nFreq == 0) return;

    uint32_t Div = 1193180 / nFreq;

    // Set PIT Channel 2 to Square Wave mode (0xB6)
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(Div & 0xFF));        // Low byte
    outb(0x42, (uint8_t)((Div >> 8) & 0xFF)); // High byte

    // Enable speaker gating (bits 0 and 1) on Port 0x61
    uint8_t tmp = inb(0x61);
    if ((tmp & 3) != 3) {
        outb(0x61, tmp | 3);
    }
}

void nosound(void) {
    // Disable speaker gating (clear bits 0 and 1)
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

// Fixed short delay helper
void beep(uint32_t freq, uint32_t duration) {
    play_sound(freq);
    
    // Use a smaller iteration count for testing (e.g. 100000 iterations total)
    for (volatile uint32_t i = 0; i < duration * 1000; i++) {
        __asm__ __volatile__("pause");
    }
    
    nosound();
}
