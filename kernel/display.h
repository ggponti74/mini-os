#ifndef _DISPLAY_H
#define _DISPLAY_H

#define VGA_MEMORY ((volatile uint16_t*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Default attribute: Light Grey text on Black background
#define COLOR_DEFAULT 0x07

// Highlight attribute: Bright White text on Black background
#define COLOR_WHITE 0x0F

static int cursor_x = 0;
static int cursor_y = 0;
static int current_color= COLOR_DEFAULT;

// Write a single character to COM1 serial port
void serial_putc(char c) ;

// Write a string to COM1 serial port
void serial_puts(const char* str) ;

// Print a null-terminated string
void kprint(const char* str) ;
void kputchar(char c);

void clear_screen(void) ;
void serial_clear_screen(void);

#endif
