// hal/display.h
#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include <stdint.h>

void hal_display_init(void);
void hal_display_clear(void);
void hal_display_putchar(char c);
void hal_display_write(const char* str);
void hal_display_set_cursor(uint8_t x, uint8_t y);

#endif