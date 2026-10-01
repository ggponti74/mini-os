// kernel/serial.c - COM1 serial console (115200 8N1), output + interrupt-driven input
#include <stdint.h>
#include "io.h"
#include "display.h"
#include "shell.h"

#define COM1      0x3F8
#define COM1_DATA (COM1 + 0)   // RX/TX (DLAB=0), divisor low (DLAB=1)
#define COM1_IER  (COM1 + 1)   // interrupt enable, divisor high (DLAB=1)
#define COM1_FCR  (COM1 + 2)
#define COM1_LCR  (COM1 + 3)
#define COM1_MCR  (COM1 + 4)
#define COM1_LSR  (COM1 + 5)

extern void pic_send_eoi(unsigned char irq);

void serial_init(void) {
    outb(COM1_IER, 0x00);   // disable UART interrupts while configuring
    outb(COM1_LCR, 0x80);   // DLAB on
    outb(COM1_DATA, 0x01);  // divisor 1 = 115200 baud
    outb(COM1_IER, 0x00);
    outb(COM1_LCR, 0x03);   // 8 data bits, no parity, 1 stop bit
    outb(COM1_FCR, 0xC7);   // enable + clear FIFOs, 14-byte threshold
    outb(COM1_MCR, 0x0B);   // DTR, RTS, OUT2 (OUT2 gates the UART IRQ line)
    outb(COM1_IER, 0x01);   // interrupt on received data
}

void serial_putc(char c) {
    // Bounded wait so a missing UART can never hang the kernel
    for (int i = 0; i < 100000 && !(inb(COM1_LSR) & 0x20); i++) { }
    outb(COM1_DATA, (uint8_t)c);
}

void serial_puts(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putc(str[i]);
    }
}

void serial_clear_screen(void) {
    serial_puts("\x1b[2J\x1b[H");   // ANSI: clear screen, cursor home
}

// Called from irq4_serial_stub (kernel/interrupts.asm)
void serial_handler(void) {
    static int esc_state = 0;      // 0 = normal, 1 = after ESC, 2 = inside CSI

    while (inb(COM1_LSR) & 0x01) {
        uint8_t c = inb(COM1_DATA);

        // Swallow escape sequences (arrow keys etc.) instead of typing them
        if (esc_state == 0 && c == 0x1B) { esc_state = 1; continue; }
        if (esc_state == 1) { esc_state = (c == '[' || c == 'O') ? 2 : 0; continue; }
        if (esc_state == 2) { if (c >= 0x40 && c <= 0x7E) esc_state = 0; continue; }

        if (c == '\r' || c == '\n') c = '\n';
        else if (c == 0x7F || c == 0x08) c = '\b';

        shell_input_char((char)c);
    }

    pic_send_eoi(4);
}
