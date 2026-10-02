#include <stdint.h>

#define VGA_MEMORY ((volatile uint16_t*) 0xB8000)
#define WHITE_ON_BLACK 0x0F

void vga_clear_screen(void) {
    volatile uint16_t *buffer = VGA_MEMORY;
    uint16_t blank = (WHITE_ON_BLACK << 8) | ' ';
    
    for (int i = 0; i < 80 * 25; i++) {
        buffer[i] = blank;
    }
}

void vga_print_at(const char *str, int col, int row) {
    volatile uint16_t *buffer = VGA_MEMORY;
    int index = row * 80 + col;

    for (int i = 0; str[i] != '\0'; i++) {
        buffer[index + i] = (WHITE_ON_BLACK << 8) | (uint8_t)str[i];
    }
}
