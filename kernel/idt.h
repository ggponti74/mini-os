// kernel/idt.h
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT Entry Structure (8 bytes)
struct idt_entry {
    uint16_t base_low;   // Lower 16 bits of handler address
    uint16_t sel;        // Kernel code segment selector (0x08 / CODE_SEG)
    uint8_t  always0;    // Always set to 0
    uint8_t  flags;      // Type and attributes (0x8E for 32-bit Interrupt Gate)
    uint16_t base_high;  // Upper 16 bits of handler address
} __attribute__((packed));

// Pointer structure loaded by LIDT instruction
struct idt_ptr {
    uint16_t limit;      // Size of IDT array minus 1
    uint32_t base;       // Base address of IDT array
} __attribute__((packed));

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_init(void);

// Inform the compiler/editor that this symbol exists in assembly
extern void isr_default_stub(void);

#endif