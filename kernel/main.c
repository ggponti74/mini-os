#include "display.h"
#include "sound.h"

// Main

void kernel_main(void) {
    // 1. Write to VGA Video Memory (0xB8000)
    volatile char* vga = (volatile char*) 0xB8000;
    vga[0] = 'H';
    vga[1] = 0x0F;

    clear_vga_screen();

    // 2. Write directly to Terminal via COM1 Serial Output
    serial_puts(" \nWelcome to the mini-os\n");
 
    // Keep kernel alive
    while (1) {
        __asm__ volatile("hlt");
    }
}
