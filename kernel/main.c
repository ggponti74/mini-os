#include "display.h"
#include "pic.h"
#include "idt.h"
#include "keyboard.h"
#include "sound.h"
#include "shell.h"

// Inline helper for serial port diagnostic logging
static inline void serial_outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_outb(0x3F8, str[i]);
    }
}

void kernel_main(void) {
    // 1. Disable interrupts during baseline initialization
    __asm__ volatile ("cli");

    // 2. Wipe SeaBIOS boot messages and reset cursor to (0,0)
    clear_screen();
    kprint("Screen cleaned...\n");

    // initializing display
    display_init();

    // Remap 8259 PIC vectors to 0x20-0x28 (avoids CPU exception conflicts)
    kprint("Initializing PIC...\n");
    pic_remap();

    // Load Interrupt Descriptor Table (IDT)
    kprint("Initializing interrupt descriptor table...\n");
    idt_init();
 
    // Initialize Keyboard Driver (IRQ1)
    kprint("Initializing keyboard...\n");
    keyboard_init();
 
    // Optional: Brief startup chime (440 Hz for 50 ms)
    kprint("Initializing sound...\n");
    beep(440, 10);

    //-enable hardware interrupts to process keypress events safely
    __asm__ volatile ("sti");
    kprint("System ready, starting shell...\n\n");

    shell_init();

    update_status_bar();
    
    // 9. Main kernel event loop
    while (1) {
        update_status_bar();
        __asm__ volatile ("hlt");
    }
}