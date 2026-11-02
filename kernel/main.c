#include "display.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include "idt.h"

#include "keyboard.h"
#include "memory.h"
#include "memory_layout.h"
#include "hal/network/network.h"
#include "hal/timer.h"
#include "pic.h"
#include "shell.h"
#include "sound.h"
#include "sys/metrics.h"

extern char kernel_end_symbol[];

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

  display_init();

  clear_screen();
  kprint_color("Initializing screen...\n", COLOR_DEFAULT);

  kprint_color("Initializing PIC...\n", COLOR_DEFAULT);
  pic_remap();

  kprint_color("Initializing interrupt descriptor table...\n", COLOR_DEFAULT);
  idt_init();

  kprint_color("Initializing keyboard...\n", COLOR_DEFAULT);
  keyboard_init();

  kprint_color("Initializing timer...\n", COLOR_DEFAULT);
  hal_timer_init(1000);

  kprint_color("Initializing sound...\n", COLOR_DEFAULT);
  sound_init();

  kprint_color("Initializing virtual file system and RAM disk...\n",
               COLOR_DEFAULT);
  fs_root = initrd_init(); // Assigns the root VFS node so shell commands like
  // touch/cat can use it

  kprint_color("Initializing PMM...\n", COLOR_DEFAULT);
  pmm_init(MINI_OS_RAM_SIZE, (uintptr_t)kernel_end_symbol);

  kprint_color("Initializing memory...\n", COLOR_DEFAULT);
  uintptr_t heap_start = pmm_get_usable_start();
  kheap_init(heap_start, KERNEL_HEAP_SIZE);
  pmm_reserve_range(heap_start, KERNEL_HEAP_SIZE);
  pmm_reserve_range(KERNEL_STACK_BASE,
                    KERNEL_STACK_TOP - KERNEL_STACK_BASE);
  pmm_reserve_range(INITRD_FILE_STORAGE_BASE, INITRD_MAX_FILE_SIZE);
  pmm_reserve_range(USER_APP_LOAD_BASE, USER_APP_MAX_SIZE);

  kprint_color("Initializing network support...\n", COLOR_DEFAULT);
  hal_network_init();
  int dhcp_status = hal_network_configure_dhcp();

  __asm__ volatile("sti" ::: "memory");

  kprint_color("System ready, starting shell...\n\n", COLOR_DEFAULT);
  shell_init();
  update_status_bar();
  display_set_fs_indicator(FS_IND_IDLE, FS_IND_IDLE);

  while (1) {
    shell_run_pending_command();
    update_status_bar();
    cpu_idle_hook();
  }
}
