#include <stdint.h>
#include "sound.h"
#include "io.h"

static uint16_t pit_channel0_count(void) {
    outb(0x43, 0x00);
    uint16_t count = inb(0x40);
    count |= (uint16_t)inb(0x40) << 8;
    return count;
}

static void wait_ms(uint32_t milliseconds) {
    const uint16_t divisor = 1193;
    uint32_t remaining = milliseconds * divisor;
    uint32_t poll_budget = milliseconds * 10000;

    outb(0x43, 0x34);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));

    uint16_t previous = pit_channel0_count();

    while (remaining > 0 && poll_budget > 0) {
        uint16_t current = pit_channel0_count();
        uint32_t elapsed = previous >= current
                               ? previous - current
                               : previous + (uint32_t)divisor - current;
        if (elapsed > 0) {
            if (elapsed >= remaining) {
                return;
            }
            remaining -= elapsed;
            previous = current;
        }
        poll_budget--;
    }
}

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
    wait_ms(duration);
    nosound();
}

void sound_init(void) {
    // Enable speaker gate on Port 0x61 (set bits 0 and 1)
    uint8_t tmp = inb(0x61);
    if ((tmp & 3) != 3) {
        outb(0x61, tmp | 3);
    }
    nosound();
}