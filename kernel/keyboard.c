#include "keyboard.h"
#include "display.h"
#include "io.h"
#include "pic.h"
#include "shell.h"

static int extended_scancode = 0;
static int shift_pressed = 0;

volatile int waiting_for_pause = 0;
volatile int pause_extended = 0;

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

void keyboard_init(void) {
  // 1. Unmask IRQ1 on the Master PIC (Bit 1 = 0)
  uint8_t mask = inb(0x21);
  outb(0x21, mask & ~(1 << 1));

  // 2. Flush any residual boot/SeaBIOS bytes out of the PS/2 data port (0x60)
  while (inb(0x64) & 0x01) {
    inb(0x60);
  }
}

void set_keyboard_pause_state(int state) { waiting_for_pause = state; }
int get_keyboard_pause_state() { return waiting_for_pause; }

void keyboard_handler(void) {

  // 1. MUST read data port to notify PS/2 controller
  uint8_t scancode = inb(0x60);
  outb(0xE9, scancode);

  // If we are paused, swallow incoming keys until we catch a key release (break
  // code)
  static int pause_extended = 0;

  if (waiting_for_pause) {

    // Handle extended prefix
    if (scancode == 0xE0) {
      pause_extended = 1;
      pic_send_eoi(1);
      return;
    }

    // If we saw E0 previously, this is the actual key
    if (pause_extended) {
      pause_extended = 0;

      if (!(scancode & 0x80)) {
        waiting_for_pause = 0; // exit pause mode
      }

      pic_send_eoi(1);
      return;
    }

    // Non-extended key: same rule
    if (!(scancode & 0x80)) {
      waiting_for_pause = 0;
    }

    pic_send_eoi(1);
    return;
  }

  // 2. Handle the 0xE0 prefix byte
  if (scancode == 0xE0) {
    extended_scancode = 1;
    pic_send_eoi(1);
    return;
  }

  // 3. Handle Key Presses (Make Codes)
  if (!(scancode & 0x80)) {
    if (extended_scancode) {
      extended_scancode = 0; // Consume extended flag immediately

      if (scancode == 0x48) {
        shell_handle_key_up();
      } else if (scancode == 0x50) {
        shell_handle_key_down();
      } else if (scancode == 0x4B) {
        shell_handle_key_left();
      } else if (scancode == 0x4D) {
        shell_handle_key_right();
      } else if (scancode == 0x52) {
        shell_handle_key_insert();
      }
    } else {
      // Standard non-extended key presses
      if (scancode == 0x48) {
        shell_handle_key_up();
      } else if (scancode == 0x50) {
        shell_handle_key_down();
      } else if (scancode == 0x4B) {
        shell_handle_key_left();
      } else if (scancode == 0x4D) {
        shell_handle_key_right();
      } else if (scancode == 0x52 || scancode == 0xD2) {
        shell_handle_key_insert();
      } else if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
      } else if (scancode < 128) {
        char ascii = shift_pressed ? scancode_ascii_uppercase[scancode]
                                   : scancode_ascii_lowercase[scancode];
        if (ascii != 0) {
          shell_input_char(ascii);
        }
      }
    }
  }
  // 4. Handle Key Releases (Break Codes)
  else {
    if (extended_scancode) {
      extended_scancode = 0; // Clear extended flag on release bytes like E0 D2
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
