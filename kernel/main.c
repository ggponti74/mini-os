#include "display.h"
#include "sound.h"
#include "io.h"

void kernel_main(void) {
    clear_screen(); // Resets cursor_x = 0, cursor_y = 0 and fills 0xB8000 with blank spaces
    kprint("Mini-OS Kernel Active!\n");
    
    play_sound(440);
    while (1) { __asm__ __volatile__("hlt"); }
}