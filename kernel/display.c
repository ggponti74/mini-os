#include "display.h"


// Write a byte to an I/O port
void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a byte from an I/O port
unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Write a single character to COM1 serial port
void serial_putc(char c) {
    outb(0x3F8, c);
}

// Write a string to COM1 serial port
void serial_puts(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putc(str[i]);
    }
}

/// Clear ANSI terminal screen & move cursor to top-left (Row 1, Col 1)
void serial_clear_screen(void) {
    serial_puts("\033[2J\033[H");
}

// Clear VGA memory buffer (0xB8000) for graphical displays
void clear_vga_screen(void) {
    volatile char* vga = (volatile char*) 0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = 0x07; // Light grey on black
    }
}

// Print a single character with automatic newline and scrolling support
void kputchar(char c, char color)
{
    volatile char *vga = VGA_MEMORY;

    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else
    {
        int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        vga[offset] = c;
        vga[offset + 1] = color;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH)
        {
            cursor_x = 0;
            cursor_y++;
        }
    }

    // Basic vertical bounds check (reset to top if we hit the bottom row)
    if (cursor_y >= VGA_HEIGHT)
    {
        cursor_y = 0;
    }
}

// Print a null-terminated string
void kprint(const char *str, char color)
{
    int i = 0;
    while (str[i] != '\0')
    {
        kputchar(str[i], color);
        i++;
    }
}
