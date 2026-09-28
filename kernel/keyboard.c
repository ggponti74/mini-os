// kernel/keyboard.c
#include "keyboard.h"
#include "shell.h" // Add shell header
#include "io.h"  

extern void pic_send_eoi(unsigned char irq);

static int shift_pressed = 0;

static const char scancode_ascii_lowercase[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
     0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
   '*',   0, ' '
};

static const char scancode_ascii_uppercase[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
     0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
     0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
   '*',   0, ' '
};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        uint8_t released_code = scancode & 0x7F;
        if (released_code == 0x2A || released_code == 0x36) {
            shift_pressed = 0;
        }
    } else {
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
        } else {
            char ascii = shift_pressed ? scancode_ascii_uppercase[scancode] 
                                       : scancode_ascii_lowercase[scancode];
            if (ascii != 0) {
                // Route input directly into the command shell
                shell_input_char(ascii);
            }
        }
    }

    pic_send_eoi(1);

    outb(0x20, 0x20); // Send EOI to Master PIC
}