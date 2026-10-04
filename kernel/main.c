#include "display.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include "idt.h"
#include "keyboard.h"
#include "pic.h"
#include "shell.h"
#include "sound.h"

uint32_t total_memory_pages = 0;
uint32_t free_memory_pages = 0;

// Inline helper for serial port diagnostic logging
static inline void serial_outb(unsigned short port, unsigned char val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_print(const char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    serial_outb(0x3F8, str[i]);
  }
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void kernel_main(void) {
    // Disable interrupts immediately upon entry
    __asm__ __volatile__("cli");

    // Write to debug port
    outb(0xE9, 'K');

    // Draw to VGA memory
    volatile uint16_t* vga = (volatile uint16_t*) 0xB8000;
    vga[0] = 0x1F00 | 'K';

    while (1) {
        __asm__ __volatile__("hlt");
    }
}

// void kernel_main(void) {
//   // Example for 16 MB RAM using 4KB pages: (16 * 1024 * 1024) / 4096 = 4096
//   // pages
//   total_memory_pages = 4096;
//   free_memory_pages = 3584; // Update as pages are allocated/freed

//   // Disable interrupts during baseline initialization
//   __asm__ volatile("cli");

//   // Wipe SeaBIOS boot messages and reset cursor to (0,0)
//   clear_screen();
//   kprint("Screen cleaned...\n");

//   // initializing display
//   display_init();

//   // Remap 8259 PIC vectors to 0x20-0x28 (avoids CPU exception conflicts)
//   kprint("Initializing PIC...\n");
//   pic_remap();

//   // Load Interrupt Descriptor Table (IDT)
//   kprint("Initializing interrupt descriptor table...\n");
//   idt_init();

//   // Initialize Keyboard Driver (IRQ1)
//   kprint("Initializing keyboard...\n");
//   keyboard_init();

//   // Optional: Brief startup chime (440 Hz for 50 ms)
//   kprint("Initializing sound...\n");
//   beep(440, 10);

//   // Initialize VFS root from RAMDisk
//   kprint("Initializing virtual file system and RAM disk...\n");
//   vfs_node_t *fs_root = initrd_init();

//   // Re-enable hardware interrupts to process keypress events safely
//   __asm__ volatile("sti");
//   kprint("System ready, starting shell...\n\n");

//   shell_init();

//   update_status_bar();

//   // 9. Main kernel event loop
//   while (1) {
//     update_status_bar();
//     __asm__ volatile("hlt");
//   }
// }
