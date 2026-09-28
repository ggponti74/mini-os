// kernel/display.c
#include <stdint.h>
#include "display.h"
#include "io.h"

void clear_screen(void) {
    uint16_t blank = ' ' | (current_color << 8);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void kputchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_MEMORY[index] = (uint16_t)c | ((uint16_t)current_color << 8);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= VGA_HEIGHT) {
        clear_screen();
    }
}

void kprint(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        kputchar(str[i]);
    }
}