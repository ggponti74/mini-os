// kernel/pic.c
#include <stdint.h>
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

// Remap PIC offsets
void pic_remap(int offset1, int offset2) {
    uint8_t a1 = inb(PIC1_DATA); // Save current masks
    uint8_t a2 = inb(PIC2_DATA);

    // ICW1: Start initialization sequence
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    // ICW2: Vector offsets
    outb(PIC1_DATA, offset1); // Master PIC offset (0x20)
    outb(PIC2_DATA, offset2); // Slave PIC offset (0x28)

    // ICW3: Cascading setup
    outb(PIC1_DATA, 4); // Tell Master PIC that Slave PIC is at IRQ2 (0000 0100b)
    outb(PIC2_DATA, 2); // Tell Slave PIC its cascade identity (0000 0010b)

    // ICW4: 8086/88 mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Restore saved interrupt masks
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);

    // Read current Master PIC mask (port 0x21) and clear bit 1 (IRQ1)
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~(1 << 1));
}

// Unmask IRQ1 (Keyboard) on Master PIC
void pic_unmask_keyboard(void) {
    uint8_t current_mask = inb(PIC1_DATA);
    outb(PIC1_DATA, current_mask & ~(1 << 1)); // Clear bit 1 (IRQ1)
}

// Send End-of-Interrupt signal to PIC
void pic_send_eoi(unsigned char irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, 0x20);
    }
    outb(PIC1_COMMAND, 0x20);
}