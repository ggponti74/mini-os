#include "idt.h"

#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

// Assembly routine defined in kernel/interrupts.asm or kernel/boot.asm
extern void idt_load(uint32_t idt_ptr_addr);
extern void isr_default_stub(void);
extern void irq1_keyboard_stub(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base  = (uint32_t)&idt;

    // Set fallback handler for all 256 vectors
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint32_t)isr_default_stub, 0x08, 0x8E);
    }

    // Load IDT into CPU IDTR register
    idt_load((uint32_t)&idtp);
}