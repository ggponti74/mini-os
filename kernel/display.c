#include "display.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static int cursor_x = 0;
static int cursor_y = 0;

// Scroll the screen up by one row when reaching the bottom
static void scroll_screen(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }

    uint16_t blank = (COLOR_DEFAULT << 8) | ' ';
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }

    cursor_y = VGA_HEIGHT - 1;
}

void clear_screen(void) {
    uint16_t blank = (COLOR_DEFAULT << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void kputchar(char c, uint8_t color) {
    // Default fallback if zero color attribute is passed
    if (color == 0) {
        color = COLOR_DEFAULT;
    }

    // Special control character processing
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | ' ';
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
            VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | ' ';
        }
    } else {
        // Render visible ASCII character to 0xB8000
        VGA_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | (uint8_t)c;
        cursor_x++;
    }

    // Line wrap
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    // Vertical scroll trigger
    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen();
    }
}

void kprint_color(const char *str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kputchar(str[i], color);
    }
}
