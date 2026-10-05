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

static inline void serial_outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_print(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_outb(0x3F8, str[i]);
    }
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void kernel_main(void) {
    __asm__ volatile ("cli");
    outb(0xE9, 'K');

    clear_screen();
    kprint("Screen cleaned...\n");
    display_init();

    kprint("Initializing PIC...\n");
    pic_remap();

    kprint("Initializing interrupt descriptor table...\n");
    idt_init();

    kprint("Initializing keyboard...\n");
    keyboard_init();

    kprint("Initializing sound...\n");
    beep(440, 10);

    kprint("Initializing virtual file system and RAM disk...\n");
    vfs_node_t *fs_root = initrd_init();
    (void)fs_root;

    __asm__ volatile ("sti");
    kprint("System ready, starting shell...\n\n");
    shell_init();
    update_status_bar();

    while (1) {
        update_status_bar();
        __asm__ volatile ("hlt");
    }
}
