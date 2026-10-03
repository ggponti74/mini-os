#include "display.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = COLOR_DEFAULT; // Default attribute (0x0F White on Black)

// Forward declarations
void draw_cursor(void);
void erase_cursor(void);

// Helper: Scroll screen up by 1 row when hitting bottom of buffer
static void scroll_screen(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }

    uint16_t blank = ((uint16_t)current_color << 8) | ' ';
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }

    cursor_y = VGA_HEIGHT - 1;
}

// Draw non-blinking inverted block/character highlight cursor
void draw_cursor(void) {
    int index = cursor_y * VGA_WIDTH + cursor_x;
    uint16_t current_cell = VGA_MEMORY[index];
    char c = (char)(current_cell & 0xFF);
    if (c == ' ' || c == 0) c = '_'; // Draw underscore on blank space

    uint8_t cursor_color = 0x70; // Inverted attribute (Black text on White background)
    VGA_MEMORY[index] = ((uint16_t)cursor_color << 8) | (uint8_t)c;
}

// Erase cursor highlight and restore standard character attribute
void erase_cursor(void) {
    int index = cursor_y * VGA_WIDTH + cursor_x;
    uint16_t current_cell = VGA_MEMORY[index];
    char c = (char)(current_cell & 0xFF);
    if (c == '_') c = ' ';

    VGA_MEMORY[index] = ((uint16_t)current_color << 8) | (uint8_t)c;
}

void clear_screen(void) {
    uint16_t blank = ((uint16_t)current_color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
    draw_cursor();
}

void kputchar_color(const char c, uint8_t color) {    // 1. Fallback to current_color if 0 is passed
    if (color == 0) {
        color = current_color;
    } else {
        current_color = color; // Update active color
    }

    // 2. Remove cursor highlight from current position
    erase_cursor();

    // 3. Process character
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
            int index = cursor_y * VGA_WIDTH + cursor_x;
            VGA_MEMORY[index] = ((uint16_t)color << 8) | ' ';
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
            int index = cursor_y * VGA_WIDTH + cursor_x;
            VGA_MEMORY[index] = ((uint16_t)color << 8) | ' ';
        }
    } else {
        int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_MEMORY[index] = ((uint16_t)color << 8) | (uint8_t)c;
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

    // 4. Draw cursor highlight at new position
    draw_cursor();
}

void kprint_color(const char *str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        kputchar_color(str[i], color);
    }
}