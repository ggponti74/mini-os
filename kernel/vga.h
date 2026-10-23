#ifndef VGA_H
#define VGA_H

void vga_clear_screen(void);
void vga_print_at(const char *str, int col, int row);

#endif // VGA_H