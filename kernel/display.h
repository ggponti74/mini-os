#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define VGA_WIDTH     80
#define VGA_HEIGHT    25
#define COLOR_DEFAULT 0x0F // White on Black

void clear_screen(void);
void kputchar(char c, uint8_t color);
void kprint_color(const char *str, uint8_t color);

// Default parameter wrapper for kprint
static inline void kprint(const char *str) {
    kprint_color(str, COLOR_DEFAULT);
}

#endif // DISPLAY_H