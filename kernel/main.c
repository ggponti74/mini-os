// kernel/main.c
#include <stdint.h>

#include"idt.h"

// Forward declarations for display routines
void clear_screen(void);
void kprint(const char* str);

// Forward declarations for PIC & IDT routines
void pic_remap(int offset1, int offset2);
void idt_init(void);

extern void isr_default_stub(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

void kernel_main(void) {
    clear_screen();

    kprint("mini-os kernel initialized\n");
    kprint("--------------------------\n");

    // 1. Remap 8259 PIC vectors to 0x20-0x27 and 0x28-0x2F
    pic_remap(0x20, 0x28);
    kprint("PIC Remapped (Master: 0x20, Slave: 0x28)\n");

    // 2. Load IDT structure
    idt_init();

    // 3. Set default handler across all 256 interrupt gates
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)isr_default_stub, 0x08, 0x8E);
    }
    kprint("IDT Loaded successfully!\n");

    // 4. Enable CPU interrupts
    __asm__ volatile("sti");
    kprint("Interrupts enabled (STI). System ready.\n");

    while (1) {
        __asm__ volatile("hlt");
    }
}