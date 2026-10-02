#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// Master and Slave PIC Base I/O Ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20

#define ICW1_INIT    0x10  // <--- Defined here in pic.c
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01

// Function Prototypes
void pic_remap(void);
void irq_mask(uint8_t irq_line);
void irq_unmask(uint8_t irq_line);
void pic_send_eoi(uint8_t irq);

#endif // PIC_H
