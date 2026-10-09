#include "keyboard.h"
#include "display.h"
#include "shell.h"
#include "io.h"
#include "pic.h"

static int extended_scancode = 0;
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

void keyboard_init(void) {
    // 1. Unmask IRQ1 on the Master PIC (Bit 1 = 0)
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~(1 << 1));

    // 2. Flush any residual boot/SeaBIOS bytes out of the PS/2 data port (0x60)
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
}

void keyboard_handler(void) {
    // MUST read data port to notify PS/2 controller
    uint8_t scancode = inb(0x60); 
outb(0xE9, scancode);
    if (scancode == 0xE0) {
        extended_scancode = 1;
        pic_send_eoi(1);
        return;
    }

    if (!(scancode & 0x80)) { // Key press
        if (extended_scancode) {
            extended_scancode = 0;
            if (scancode == 0x48) {
                shell_handle_key_up();
            } else if (scancode == 0x50) {
                shell_handle_key_down();
            }
        } else if (scancode == 0x48) {
            shell_handle_key_up();
        } else if (scancode == 0x50) {
            shell_handle_key_down();
        } else
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
        } else if (scancode < 128) {
            char ascii = shift_pressed ? scancode_ascii_uppercase[scancode]
                                       : scancode_ascii_lowercase[scancode];
            if (ascii != 0) {
                shell_input_char(ascii);
            }
        }
    } else { // Key release
        if (extended_scancode) {
            extended_scancode = 0;
        } else {
            uint8_t released = scancode & 0x7F;
            if (released == 0x2A || released == 0x36) {
                shift_pressed = 0;
            }
        }
    }

    // MANDATORY: Always send EOI at the very end of handler
    pic_send_eoi(1);
}