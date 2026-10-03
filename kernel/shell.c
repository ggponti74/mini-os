// kernel/shell.c
#include "shell.h"
#include "string.h"
#include "display.h"
#include "sound.h"

#define COLOR_PROMPT 0x0B // Light Cyan
#define COLOR_WHITE 0x0F

static char command_buffer[MAX_BUFFER_SIZE];
static int buffer_index = 0;

struct shell_command {
  const char *name;
  const char *description;
  void (*function)(void);
};

static void command_about(void);
static void command_beep(void);
static void command_clear(void);
static void command_help(void);
static void command_restart(void);
static void command_shutdown(void);
static void command_test(void);

static const struct shell_command commands[] = {
    {"about", "Show operating system info", command_about},
    {"beep", "Play a beep sound", command_beep},
    {"clear", "Clear the screen", command_clear},
    {"help", "Display this help message", command_help},
    {"restart", "Restart the system", command_restart},
    {"shutdown", "Shut down the system", command_shutdown},
    {"test", "Run diagnostic test", command_test},
};

static void print_prompt(void) {
  // CORRECT:
  kprint("mini-os> ");             // Uses default COLOR_DEFAULT
}

void process_command(const char *cmd) {
    if (cmd[0] == '\0') {
        return; // Empty command
    }

    for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            commands[i].function();
            return;
        }
    }

    kprint("Unknown command: ");
    kprint(cmd);
    kprint("\nType 'help' for available commands.\n");
}

static void command_beep(void) {
  beep(440, 50);
  kprint("Beeped!\n");
}

static void command_help(void) {
  kprint("Available commands:\n");

  for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    unsigned int name_length = 0;
    kprint("  ");
    kprint(commands[i].name);
    while (commands[i].name[name_length] != '\0')
      name_length++;
    while (name_length++ < 10)
      kprint(" ");
    kprint(" - ");
    kprint(commands[i].description);
    kprint("\n");
  }
}

static void command_clear(void) {
  clear_screen();
}

static void command_about(void) {
  kprint("mini-os v1.0 - A lightweight 32-bit x86 kernel built from scratch.\n");
}

static void command_test(void) {
  kprint("System diagnostic OK.\n");
}

static void command_shutdown(void) {
    // For newer QEMU versions (QEMU ACPI debug exit)
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0x604));
    
    // For older QEMU / Bochs
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x31), "Nd"((uint16_t)0xB004));
}

static void command_restart(void) {
    __asm__ volatile (
        "movb $0xFE, %%al\n\t"
        "outb %%al, $0x64\n\t" // Write 0xFE to the command port 0x64
        : : : "al"
    );
    
    // If that fails, halt the CPU
    while(1) { __asm__ volatile("hlt"); }
}

// Simple command processor
static void execute_command(void) {
  command_buffer[buffer_index] = '\0'; // Null-terminate string

  kputchar_color('\n', COLOR_DEFAULT);
  process_command(command_buffer);

  // Reset buffer for next command
  buffer_index = 0;
  print_prompt();
}

void shell_init(void) {
  buffer_index = 0;
  print_prompt();
}

void shell_input_char(char c) {
  if (c == '\n') {
    execute_command();
  } else if (c == '\b') {
    if (buffer_index > 0) {
      buffer_index--;
      kputchar_color('\b', COLOR_DEFAULT); // Erase character on screen
    }
  } else if (c >= ' ' && c <= '~') { // Printable ASCII characters
    if (buffer_index < MAX_BUFFER_SIZE - 1) {
      command_buffer[buffer_index++] = c;
      kputchar_color(c, COLOR_DEFAULT);
    }
  }
}