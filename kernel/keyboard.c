#include "keyboard.h"
#include "io.h"       // for inb() and outb()
#include "pic.h"      // for pic_send_eoi()
#include "shell.h"

extern void irq1_keyboard_stub(void);
extern void pic_send_eoi(unsigned char irq);

static char cmd_buf[CMD_BUFFER_SIZE];
static int cmd_len = 0;
static int shift_pressed = 0;

static const char scancode_ascii_lowercase[128] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};

static const char scancode_ascii_uppercase[128] = {
    0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' '};

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    if (!(scancode & 0x80)) { // Key press
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
        } else if (scancode < 128) {
            char ascii = shift_pressed ? scancode_ascii_uppercase[scancode] 
                                       : scancode_ascii_lowercase[scancode];

<<<<<<< HEAD
            if (ascii != 0)
                shell_input_char(ascii);
=======
            if (ascii == '\n') {
                kputchar_color('\n',COLOR_DEFAULT);
                cmd_buf[cmd_len] = '\0'; // Null-terminate command string
                
                // Execute command
                process_command(cmd_buf);
                
                // Reset buffer and print new prompt
                cmd_len = 0;
                kprint("mini-os> ");
            } else if (ascii == '\b') {
                if (cmd_len > 0) {
                    cmd_len--;
                    kputchar_color('\b', COLOR_DEFAULT); // Erase character on screen
                }
            } else if (ascii != 0 && cmd_len < CMD_BUFFER_SIZE - 1) {
                cmd_buf[cmd_len++] = ascii;
                kputchar_color(ascii, COLOR_DEFAULT);
            }
>>>>>>> d7c89f2 (Fixed shell)
        }
    } else { // Key release
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {
            shift_pressed = 0;
        }
    }

    pic_send_eoi(1);
}

void keyboard_init(void) {
  // 1. Map IDT Vector 0x21 (IRQ1) to assembly stub
  idt_set_gate(0x21, (uint32_t)irq1_keyboard_stub, 0x08, 0x8E);

  // 2. Unmask IRQ1 on Master PIC (port 0x21)
  uint8_t mask = inb(0x21);
  outb(0x21, mask & ~(1 << 1));
}