#include "display.h"
#include "sound.h"
#include "io.h"

void kernel_main(void) {
    __asm__ __volatile__("cli");

    // Debug output to log
    outb(0xE9, 'K');

    // Direct poke: Draw 'O' and 'K' in Bright White on Black at (0,0)
    volatile unsigned short *vga = (volatile unsigned short *)0xB8000;
    vga[0] = (0x0F << 8) | 'O';
    vga[1] = (0x0F << 8) | 'K';

    outb(0xE9, '1');

    while (1) {
        __asm__ __volatile__("hlt");
    }
}