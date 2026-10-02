// kernel/shell.c
#include "display.h"
#include "shell.h"

#define COLOR_PROMPT 0x0B // Light Cyan
#define COLOR_WHITE 0x0F

// External VGA / Terminal functions from main.c
// extern void kputchar(char c, char color);

// void kprint(const char *str);
// extern void kprint(const char *str, char color = COLOR_PROMPT);
// extern void clear_screen(void);

static char command_buffer[MAX_BUFFER_SIZE];
static int buffer_index = 0;

static void print_prompt(void) {
  // CORRECT:
  kprint("mini-os> ");             // Uses default COLOR_DEFAULT
}

// Compare two strings for equality
static int strcmp(const char *s1, const char *s2) {
  int i = 0;
  while (s1[i] != '\0' && s2[i] != '\0') {
    if (s1[i] != s2[i])
      return s1[i] - s2[i];
    i++;
  }
  return s1[i] - s2[i];
}

// Simple command processor
static void execute_command(void) {
  command_buffer[buffer_index] = '\0'; // Null-terminate string

  kputchar('\n', COLOR_DEFAULT);

  if (buffer_index == 0) {
    // Empty command (user just hit enter)
    print_prompt();
    return;
  }

  if (strcmp(command_buffer, "help") == 0) {
    kprint("Available commands:\n");
    kprint("  help  - Display this help message\n");
    kprint("  clear - Clear the screen\n");
    kprint("  about - Show operating system info");
  } else if (strcmp(command_buffer, "clear") == 0) {
    clear_screen();
    kprint("mini-os kernel 0.1\n------------------");
  } else if (strcmp(command_buffer, "about") == 0) {
    kprint("mini-os v0.1 - A lightweight 32-bit x86 kernel built from scratch.");
  } else {
    kprint("Unknown command: ");
    kprint(command_buffer);
  }

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
      kputchar('\b', COLOR_DEFAULT); // Erase character on screen
    }
  } else if (c >= ' ' && c <= '~') { // Printable ASCII characters
    if (buffer_index < MAX_BUFFER_SIZE - 1) {
      command_buffer[buffer_index++] = c;
      kputchar(c, COLOR_DEFAULT);
    }
  }
}