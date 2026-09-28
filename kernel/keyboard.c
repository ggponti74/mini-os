// kernel/keyboard.c
#include "keyboard.h"

// I/O Port Helper
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// External EOI helper from pic.c
extern void pic_send_eoi(unsigned char irq);

// External VGA driver function from main.c
extern void kputchar(char c, char color);

// Shift key state
static int shift_pressed = 0;

// PS/2 Set 1 Scancode Map (Unshifted)
static const char scancode_ascii_lowercase[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
     0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
   '*',   0, ' '
};

// PS/2 Set 1 Scancode Map (Shifted)
static const char scancode_ascii_uppercase[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
     0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
     0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
   '*',   0, ' '
};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60); // Read byte from PS/2 keyboard controller

    // Bit 7 set indicates a key release (break code)
    if (scancode & 0x80) {
        uint8_t released_code = scancode & 0x7F;
        if (released_code == 0x2A || released_code == 0x36) { // Left or Right Shift
            shift_pressed = 0;
        }
    } else {
        // Key press (make code)
        if (scancode == 0x2A || scancode == 0x36) { // Left or Right Shift
            shift_pressed = 1;
        } else {
            char ascii = shift_pressed ? scancode_ascii_uppercase[scancode] 
                                       : scancode_ascii_lowercase[scancode];
            if (ascii != 0) {
                // Print the typed character to the screen in light grey (0x07)
                kputchar(ascii, 0x07);
            }
        }
    }

    // Send End-Of-Interrupt to Master PIC for IRQ1 (IRQ 1 = vector 0x21)
    pic_send_eoi(1);
}