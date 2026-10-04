#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((volatile uint16_t *)0xB8000)

#define COLOR_DEFAULT 0x0F // White text on Black background

void clear_screen(void);
void kputchar(char c);
void kputchar_color(const char c, uint8_t color);
void kprint_color(const char *str, uint8_t color);
void draw_status_bar(const char* datetime_str, int sound_enabled);
void update_status_bar(void);
void draw_cursor(void);
void disable_hardware_cursor(void);
void update_hardware_cursor(int, int);

// Single-argument kprint wrapper
static inline void kprint(const char *str) {
    kprint_color(str, COLOR_DEFAULT);
}

#endif // DISPLAY_H