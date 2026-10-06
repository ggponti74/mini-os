#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

#define COLOR_DEFAULT 0x0F  // white text on Black background
#define COLOR_GRAY 0x0A     // gray text on Black background

typedef enum {
    FS_IND_IDLE = 0,
    FS_IND_READ,
    FS_IND_WRITE
} fs_indicator_t;

void display_set_fs_indicator(fs_indicator_t read_state, fs_indicator_t write_state);
void display_set_silent(int silent);
int display_is_silent(void);
void display_init(void);
void clear_screen(void);
void kputchar(char c);
void kputchar_color(const char c, uint8_t color);
void kprint_color(const char *str, uint8_t color);
void draw_status_bar(const char *datetime_str, int sound_enabled);
void update_status_bar(void);
//void draw_cursor(void);
void set_hardware_cursor_shape(int);
void disable_hardware_cursor(void);
void update_hardware_cursor(int, int);
void scroll_screen(uint8_t color);
void sound_set_enabled(int );
int sound_is_enabled(void);

void print_hex8(uint8_t value);
void print_hex32(uint32_t value);

// Single-argument kprint wrapper
static inline void kprint(const char *str) { kprint_color(str, COLOR_DEFAULT); }

#endif // DISPLAY_H