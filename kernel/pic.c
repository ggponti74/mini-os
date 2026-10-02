#include <stdint.h>
#include "io.h"    /* Assumes outb / inb inline functions */
#include "pic.h"

void pic_remap(void) {
    uint8_t a1, a2;

    // Save current IRQ masks
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    // Start initialization sequence in cascade mode
    outb(PIC1_COMMAND, ICW1_INIT | ICW4_8086);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW4_8086);
    io_wait();

    // Remap Master PIC vector offset to 0x20 (32) and Slave to 0x28 (40)
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    // Tell Master PIC that Slave PIC is at IRQ2 (0000 0100b)
    outb(PIC1_DATA, 0x04);
    io_wait();
    // Tell Slave PIC its cascade identity (2)
    outb(PIC2_DATA, 0x02);
    io_wait();

    // Use 8086/88 (MCS-80/85) mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

// MASK ALL IRQs (0xFF) to ensure no unhandled interrupts fire!
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void irq_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) | (1 << irq_line);
    outb(port, value);
}

void irq_unmask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);
}

void pic_send_eoi(uint8_t irq) {
    // If the interrupt came from the Slave PIC (IRQ 8–15), send EOI to Slave
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Always send EOI to Master PIC
    outb(PIC1_COMMAND, PIC_EOI);
}