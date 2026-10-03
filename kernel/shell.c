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

struct shell_command {
  const char *name;
  const char *description;
  void (*function)(void);
};

static void command_help(void);
static void command_clear(void);
static void command_about(void);

static const struct shell_command commands[] = {
    {"help", "Display this help message", command_help},
    {"clear", "Clear the screen", command_clear},
    {"about", "Show operating system info", command_about},
};

static void command_help(void) {
  kprint("Available commands:\n");

  for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    unsigned int name_length = 0;
    kprint("  ");
    kprint(commands[i].name);
    while (commands[i].name[name_length] != '\0')
      name_length++;
    while (name_length++ < 5)
      kprint(" ");
    kprint(" - ");
    kprint(commands[i].description);
    if (i + 1 < sizeof(commands) / sizeof(commands[0]))
      kprint("\n");
  }
}

static void command_clear(void) {
  clear_screen();
  kprint("mini-os kernel 1.0\n------------------");
}

static void command_about(void) {
  kprint("mini-os v1.0 - A lightweight 32-bit x86 kernel built from scratch.");
}

// Simple command processor
static void execute_command(void) {
  command_buffer[buffer_index] = '\0'; // Null-terminate string

  kputchar_color('\n', COLOR_DEFAULT);

  if (buffer_index == 0) {
    // Empty command (user just hit enter)
    print_prompt();
    return;
  }

  unsigned int i;
  for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (strcmp(command_buffer, commands[i].name) == 0) {
      commands[i].function();
      break;
    }
  }

  if (i == sizeof(commands) / sizeof(commands[0])) {
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
      kputchar_color('\b', COLOR_DEFAULT); // Erase character on screen
    }
  } else if (c >= ' ' && c <= '~') { // Printable ASCII characters
    if (buffer_index < MAX_BUFFER_SIZE - 1) {
      command_buffer[buffer_index++] = c;
      kputchar_color(c, COLOR_DEFAULT);
    }
  }
}