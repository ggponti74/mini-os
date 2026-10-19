#include "idt.h"

#define IDT_ENTRIES 256

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

// Assembly routine defined in kernel/interrupts.asm or kernel/boot.asm
extern void idt_load(uint32_t idt_ptr_addr);
extern void isr_default_stub(void);
extern void irq0_timer_stub(void);
extern void irq1_keyboard_stub(void); // Declared here
extern void syscall_stub(void);

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

    // 1. Fallback for all 256 gates
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint32_t)isr_default_stub, 0x08, 0x8E);
    }

    // 2. Map IRQ0 and IRQ1 to their handlers.
    idt_set_gate(32, (uint32_t)irq0_timer_stub, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1_keyboard_stub, 0x08, 0x8E);
    idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0xEF);

    // 3. Load IDT
    idt_load((uint32_t)&idtp);
}
