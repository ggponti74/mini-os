#include "display.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include "idt.h"
#include "keyboard.h"
#include "pic.h"
#include "shell.h"
#include "sound.h"

static inline void serial_outb(unsigned short port, unsigned char val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_print(const char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    serial_outb(0x3F8, str[i]);
  }
}

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void kernel_main(void) {
  __asm__ volatile("cli");
  outb(0xE9, 'K');

  clear_screen();
  kprint_color("Screen cleaned...\n", COLOR_DEFAULT);
  display_init();

  kprint_color("Initializing PIC...\n", COLOR_DEFAULT);
  pic_remap();

  kprint_color("Initializing interrupt descriptor table...\n", COLOR_DEFAULT);
  idt_init();

  kprint_color("Initializing keyboard...\n", COLOR_DEFAULT);
  keyboard_init();

  kprint_color("Initializing sound...\n", COLOR_DEFAULT);
  beep(440, 10);

  kprint_color("Initializing virtual file system and RAM disk...\n",
               COLOR_DEFAULT);
  fs_root = initrd_init(); // Assigns the root VFS node so shell commands like
                           // touch/cat can use it

  __asm__ volatile("sti");
  kprint_color("System ready, starting shell...\n\n", COLOR_DEFAULT);
  shell_init();
  update_status_bar();
  display_set_fs_indicator(FS_IND_IDLE, FS_IND_IDLE);

  while (1) {
    update_status_bar();
    __asm__ volatile("hlt");
  }
}
