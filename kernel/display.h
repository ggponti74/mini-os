#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)

#define COLOR_DEFAULT 0x0F // White text on Black background

void clear_screen(void);
void kputchar_color(const char c, uint8_t color);
void kprint_color(const char *str, uint8_t color);

// Single-argument kprint wrapper
static inline void kprint(const char *str) {
    kprint_color(str, COLOR_DEFAULT);
}

#endif // DISPLAY_H