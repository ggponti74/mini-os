// kernel/display.c
#include <stdint.h>
#include "display.h"

static int cursor_offset = 0;

// void clear_screen(void) {
//     volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
//     for (int i = 0; i < 80 * 25; i++) {
//         // Bright white text on black background (0x0F)
//         vga[i] = (0x0F << 8) | ' ';
//     }
//     cursor_offset = 0; // MUST reset cursor offset when screen clears!
// }

// void kprint(const char *str) {
//     volatile uint16_t *vga = (volatile uint16_t *)0xB8000;

//     for (int i = 0; str[i] != '\0'; i++) {
//         if (str[i] == '\n') {
//             // Move to start of next row (80 columns per row)
//             cursor_offset = (cursor_offset / 80 + 1) * 80;
//         } else {
//             // Attribute byte 0x0F (White on Black) + Character
//             vga[cursor_offset] = (0x0F << 8) | (uint8_t)str[i];
//             cursor_offset++;
//         }

//         // Wrap around at bottom of screen (80 * 25 = 2000 cells)
//         if (cursor_offset >= 80 * 25) {
//             cursor_offset = 0;
//         }
//     }
// }
