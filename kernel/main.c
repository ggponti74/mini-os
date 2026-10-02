#include "display.h"

void kernel_main(void) {
    // 1. Wipe SeaBIOS and bootloader text from video memory
    clear_screen();

    // 2. Print clean status messages at top-left
    kprint("========================================\n");
    kprint("  Mini-OS 32-bit Kernel Running (ISO)   \n");
    kprint("========================================\n");

    while (1) {
        __asm__ volatile ("hlt");
    }
}
